#include "SimtelEventSource.hh"
#include "ArrayEvent.hh"
#include "CameraDescription.hh"
#include "CameraGeometry.hh"
#include "LACT_hessioxxx/include/io_basic.h"
#include "LACT_hessioxxx/include/io_hess.h"
#include "SimtelFileHandler.hh"
#include "SimulatedShowerArray.hh"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"
#include <cassert>
#include "LACT_hessioxxx/include/io_history.h"
#include "LACT_hessioxxx/include/mc_tel.h"
#include "LACT_hessioxxx/include/mc_atmprof.h"
#include "Utils.hh"
#include <cstdint>
#include <thread>
#include "Utils.hh"
SimtelEventSource::SimtelEventSource(const std::string& filename, int64_t max_events, std::vector<int> subarray, bool load_simulated_showers, int gain_selector_threshold):
    EventSource(filename, max_events, subarray, load_simulated_showers),
    gain_selector_threshold(gain_selector_threshold)
{
    initialize();
}

void SimtelEventSource::open_file()
{
    simtel_file_handler = std::make_unique<SimtelFileHandler>(input_filename);
    is_stream = true;
    simtel_file_handler->read_until_event();
}
void SimtelEventSource::init_metaparam()
{
    set_metaparam();
}
void SimtelEventSource::init_atmosphere_model()
{
    atmosphere_model = TableAtmosphereModel(simtel_file_handler->atmprof->n_alt, simtel_file_handler->atmprof->alt_km, simtel_file_handler->atmprof->rho, simtel_file_handler->atmprof->thick, simtel_file_handler->atmprof->refidx_m1);
}
void SimtelEventSource::init_simulation_config()
{
    set_simulation_config();
}
void SimtelEventSource::init_subarray()
{
    subarray = SubarrayDescription();
    //Mean we will read all telescopes in the file
    if(allowed_tels.empty())
    {
        for(const auto& [tel_id, tel_index]: simtel_file_handler->tel_id_to_index) {
            allowed_tels.push_back(tel_id);
        }
    }
    std::sort(allowed_tels.begin(), allowed_tels.end());
    for(const auto& tel_id: allowed_tels) {
        set_telescope_settings(tel_id);
    }
}
void SimtelEventSource::set_simulation_config()
{
    simulation_config = SimulationConfiguration();
    simulation_config->run_number = simtel_file_handler->hsdata->run_header.run;
    simulation_config->corsika_version = simtel_file_handler->hsdata->mc_run_header.shower_prog_vers;
    simulation_config->simtel_version = simtel_file_handler->hsdata->mc_run_header.detector_prog_vers;
    simulation_config->energy_range_min = simtel_file_handler->hsdata->mc_run_header.E_range[0];
    simulation_config->energy_range_max = simtel_file_handler->hsdata->mc_run_header.E_range[1];
    simulation_config->prod_site_B_total = simtel_file_handler->hsdata->mc_run_header.B_total;
    simulation_config->prod_site_B_declination = simtel_file_handler->hsdata->mc_run_header.B_declination;
    simulation_config->prod_site_B_inclination = simtel_file_handler->hsdata->mc_run_header.B_inclination;
    simulation_config->prod_site_alt = simtel_file_handler->hsdata->mc_run_header.obsheight;
    simulation_config->spectral_index = simtel_file_handler->hsdata->mc_run_header.spectral_index;
    simulation_config->shower_prog_start = simtel_file_handler->hsdata->mc_run_header.shower_prog_start;
    simulation_config->shower_prog_id = simtel_file_handler->hsdata->mc_run_header.shower_prog_id;
    simulation_config->detector_prog_start = simtel_file_handler->hsdata->mc_run_header.detector_prog_start;
    simulation_config->n_showers = simtel_file_handler->hsdata->mc_run_header.num_showers;
    simulation_config->shower_reuse = simtel_file_handler->hsdata->mc_run_header.num_use;
    simulation_config->max_alt = simtel_file_handler->hsdata->mc_run_header.alt_range[1];
    simulation_config->min_alt = simtel_file_handler->hsdata->mc_run_header.alt_range[0];
    simulation_config->max_az = simtel_file_handler->hsdata->mc_run_header.az_range[1];
    simulation_config->min_az = simtel_file_handler->hsdata->mc_run_header.az_range[0];
    simulation_config->diffuse = simtel_file_handler->hsdata->mc_run_header.diffuse;
    simulation_config->max_viewcone_radius = simtel_file_handler->hsdata->mc_run_header.viewcone[1];
    simulation_config->min_viewcone_radius = simtel_file_handler->hsdata->mc_run_header.viewcone[0];
    simulation_config->atmosphere = simtel_file_handler->hsdata->mc_run_header.atmosphere;
    simulation_config->corsika_iact_options = simtel_file_handler->hsdata->mc_run_header.corsika_iact_options;
    simulation_config->corsika_bunchsize = static_cast<int>(simtel_file_handler->hsdata->mc_run_header.corsika_bunchsize);
    simulation_config->corsika_low_E_model = simtel_file_handler->hsdata->mc_run_header.corsika_low_E_model;
    simulation_config->corsika_high_E_model = simtel_file_handler->hsdata->mc_run_header.corsika_high_E_model;
    simulation_config->corsika_wlen_min = simtel_file_handler->hsdata->mc_run_header.corsika_wlen_min;
    simulation_config->corsika_wlen_max = simtel_file_handler->hsdata->mc_run_header.corsika_wlen_max;
}
void SimtelEventSource::set_metaparam()
{
    metaparam = Metaparam();
    metaparam->global_metadata = simtel_file_handler->global_metadata;
    metaparam->tel_metadata = simtel_file_handler->tel_metadata;
    if(simtel_file_handler->history_container.cfg_global != NULL) { 
        while(simtel_file_handler->history_container.cfg_global->next != NULL) {
            metaparam->history.push_back(std::make_pair(simtel_file_handler->history_container.cfg_global->time, simtel_file_handler->history_container.cfg_global->text));
            simtel_file_handler->history_container.cfg_global = simtel_file_handler->history_container.cfg_global->next;
        }
    }
    if(simtel_file_handler->history_container.cfg_tel != NULL) {    
        for(size_t itel = 0; itel < simtel_file_handler->history_container.ntel; itel++) {
            while(simtel_file_handler->history_container.cfg_tel[itel] != NULL) {
            metaparam->tel_history[itel].push_back(std::make_pair(simtel_file_handler->history_container.cfg_tel[itel]->time, simtel_file_handler->history_container.cfg_tel[itel]->text));
            simtel_file_handler->history_container.cfg_tel[itel] = simtel_file_handler->history_container.cfg_tel[itel]->next;
            }
        }
    }
}

void SimtelEventSource::set_telescope_settings(int tel_id)
{
    auto it = simtel_file_handler->tel_id_to_index.find(tel_id);
    if(it == simtel_file_handler->tel_id_to_index.end()) {
        spdlog::warn("Skip telescope settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    camera_name = fmt::format("{}_{}", metaparam->tel_metadata[tel_id]["CAMERA_CONFIG_NAME"], metaparam->tel_metadata[tel_id]["CAMERA_CONFIG_VERSION"]);
    auto camera_geometry = get_telescope_camera_geometry(itel);
    auto camera_readout = get_telescope_camera_readout(itel);
    auto optics = get_telescope_optics(itel);
    if(optics.effective_focal_length != 0)
    {
        camera_geometry.pix_x_fov = camera_geometry.pix_x / optics.effective_focal_length;
        camera_geometry.pix_y_fov = camera_geometry.pix_y / optics.effective_focal_length;
        camera_geometry.pix_width_fov = camera_geometry.pix_width / optics.effective_focal_length;
    }
    else
    {
        spdlog::warn("Effective focal length is not available, so using the equivalent focal length");
        camera_geometry.pix_x_fov = camera_geometry.pix_x / optics.equivalent_focal_length;
        camera_geometry.pix_y_fov = camera_geometry.pix_y / optics.equivalent_focal_length;
        camera_geometry.pix_width_fov = camera_geometry.pix_width / optics.equivalent_focal_length;
    }
    auto camera_description = CameraDescription(camera_name, std::move(camera_geometry), std::move(camera_readout));
    auto telescope_description = TelescopeDescription(std::move(camera_description), std::move(optics));
    auto telescope_position = get_telescope_position(itel);
    subarray->add_telescope(tel_id, std::move(telescope_description), telescope_position);
}
CameraGeometry SimtelEventSource::get_telescope_camera_geometry(int tel_index)
{
    int num_pixels = simtel_file_handler->hsdata->camera_set[tel_index].num_pixels;
    double* pix_x = simtel_file_handler->hsdata->camera_set[tel_index].xpix;
    double* pix_y = simtel_file_handler->hsdata->camera_set[tel_index].ypix;
    double* pix_area = simtel_file_handler->hsdata->camera_set[tel_index].area;
    int* pix_type = simtel_file_handler->hsdata->camera_set[tel_index].pixel_shape;
    double cam_rotation = simtel_file_handler->hsdata->camera_set[tel_index].cam_rot;
    return CameraGeometry(camera_name, num_pixels, pix_x, pix_y, pix_area, pix_type, cam_rotation);
}

CameraReadout SimtelEventSource::get_telescope_camera_readout(int tel_index)
{
    int num_pixels = simtel_file_handler->hsdata->pixel_set[tel_index].num_pixels;
    int n_channels = simtel_file_handler->hsdata->camera_org[tel_index].num_gains;
    int n_samples = simtel_file_handler->hsdata->pixel_set[tel_index].sum_bins;
    double sampling_rate = 1/simtel_file_handler->hsdata->pixel_set[tel_index].time_slice;
    double reference_pulse_sample_width = simtel_file_handler->hsdata->pixel_set[tel_index].ref_step;
    Eigen::MatrixXd reference_pulse_shape(n_channels, simtel_file_handler->hsdata->pixel_set[tel_index].lrefshape);
    for (int i = 0; i < n_channels; i++)
    {
        for (int j = 0; j < simtel_file_handler->hsdata->pixel_set[tel_index].lrefshape; j++)
        {
            reference_pulse_shape(i, j) = simtel_file_handler->hsdata->pixel_set[tel_index].refshape[i][j];
        }
    }
    return CameraReadout(camera_name, sampling_rate, reference_pulse_sample_width, n_channels, num_pixels, n_samples, std::move(reference_pulse_shape));
}
OpticsDescription SimtelEventSource::get_telescope_optics(int tel_index)
{
    double focal_length = simtel_file_handler->hsdata->camera_set[tel_index].flen;
    double mirror_area = simtel_file_handler->hsdata->camera_set[tel_index].mirror_area;
    int num_mirrors = simtel_file_handler->hsdata->camera_set[tel_index].num_mirrors;
    double effective_focal_length = simtel_file_handler->hsdata->camera_set[tel_index].eff_flen;
    return OpticsDescription(optics_name, num_mirrors, mirror_area, focal_length, effective_focal_length);
}
std::array<double, 3> SimtelEventSource::get_telescope_position(int tel_index)
{
    double tel_x = simtel_file_handler->hsdata->run_header.tel_pos[tel_index][0];
    double tel_y = simtel_file_handler->hsdata->run_header.tel_pos[tel_index][1];
    double tel_z = simtel_file_handler->hsdata->run_header.tel_pos[tel_index][2];
    return std::array<double, 3>{tel_x, tel_y, tel_z};
}
void SimtelEventSource::load_all_simulated_showers()
{
    // Create a temporary shower array in a new thread
    auto load_showers = [this]() {
        auto temp_file_handler = std::make_unique<SimtelFileHandler>(input_filename);
        SimulatedShowerArray temp_shower_array(simulation_config->n_showers * 20);
        
        while(temp_file_handler->only_read_mc_event()) {
            SimulatedShower shower;
            shower.energy = temp_file_handler->hsdata->mc_shower.energy;
            shower.alt = temp_file_handler->hsdata->mc_shower.altitude;
            shower.az = temp_file_handler->hsdata->mc_shower.azimuth;
            shower.core_x = temp_file_handler->hsdata->mc_event.xcore;
            shower.core_y = temp_file_handler->hsdata->mc_event.ycore;
            shower.h_first_int = temp_file_handler->hsdata->mc_shower.h_first_int;
            shower.x_max = temp_file_handler->hsdata->mc_shower.xmax;
            shower.starting_grammage = temp_file_handler->hsdata->mc_shower.depth_start;
            shower.shower_primary_id = temp_file_handler->hsdata->mc_shower.primary_id;
            shower.h_max = temp_file_handler->hsdata->mc_shower.hmax;
            temp_shower_array.push_back(shower);
        }
        
        // Copy the temporary shower array to the main shower array
        shower_array = std::move(temp_shower_array);
    };

    // Launch the loading in a separate thread
    std::thread loader_thread(load_showers);
    loader_thread.join();
}
bool SimtelEventSource::_load_next_event()
{
    return simtel_file_handler->load_next_event();
}
ArrayEvent SimtelEventSource::get_event()
{
    if (!_load_next_event()) {
        return ArrayEvent(); // If no more events, return an empty event
    }
    ArrayEvent event;
    read_simulated_showers(event);
    if(simtel_file_handler->have_true_image)
    {
        read_true_image(event);
    }
    read_monitor(event);
    read_pointing(event);
    read_adc_samples(event);
    apply_simtel_calibration(event);
    return event;
}
void SimtelEventSource::read_adc_samples(ArrayEvent& event)
{
    if(!event.r0) {
        event.r0 = R0Event();
    }
    for(const auto& tel_id: allowed_tels) {
        auto tel_index = simtel_file_handler->tel_id_to_index[tel_id];
        if(!simtel_file_handler->hsdata->event.teldata[tel_index].known)
        {
            continue;
        }
        if(simtel_file_handler->hsdata->event.teldata[tel_index].raw->known)
        {
            uint32_t* high_gain_waveform_sum = nullptr;
            uint32_t* low_gain_waveform_sum = nullptr;
            Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> high_gain_waveform;
            Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> low_gain_waveform;
            // Read ADC samples
            if(simtel_file_handler->hsdata->event.teldata[tel_index].raw->adc_known[0][0] & (1L<<1))
            {
                high_gain_waveform.resize(simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_pixels, simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_samples);
                for(int ipixel = 0; ipixel < simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_pixels; ipixel++) {
                    for(int isample = 0; isample < simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_samples; isample++) {
                        high_gain_waveform(ipixel, isample) = simtel_file_handler->hsdata->event.teldata[tel_index].raw->adc_sample[0][ipixel][isample];
                    }
                }
                spdlog::debug("ADC samples are available for tel_id: {} for first gain", tel_id);
            }
            else 
            {
                return; // We hope to get the adc samples now(maybe in the future we cam get sum or peak), so now if don't have adc samples just return
            }
            if(simtel_file_handler->hsdata->event.teldata[tel_index].raw->adc_known[1][0] & (1L<<1))
            {
                low_gain_waveform.resize(simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_pixels, simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_samples);
                spdlog::debug("ADC samples are available for tel_id: {} for second gain", tel_id);
                for(int ipixel = 0; ipixel < simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_pixels; ipixel++) {
                    for(int isample = 0; isample < simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_samples; isample++) {
                        low_gain_waveform(ipixel, isample) = simtel_file_handler->hsdata->event.teldata[tel_index].raw->adc_sample[1][ipixel][isample];
                    }
                }
            }
            if(simtel_file_handler->hsdata->event.teldata[tel_index].raw->adc_known[0][0] & (1L))
            {
                spdlog::debug("ADC sums are available for tel_id: {} for first gain", tel_id);
                high_gain_waveform_sum = &simtel_file_handler->hsdata->event.teldata[tel_index].raw->adc_sum[0][0];
                if(simtel_file_handler->hsdata->event.teldata[tel_index].raw->adc_known[1][0] & (1L))
                {
                    spdlog::debug("ADC sums are available for tel_id: {} for second gain", tel_id);
                    low_gain_waveform_sum = &simtel_file_handler->hsdata->event.teldata[tel_index].raw->adc_sum[1][0];
                }
            }
            // If no adc_sums, it will be nullptr
            event.r0->add_tel(tel_id, 
                    simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_pixels,
                    simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_samples,
                    std::move(high_gain_waveform),
                    std::move(low_gain_waveform),
                    std::move(high_gain_waveform_sum),
                    std::move(low_gain_waveform_sum));
        }
    }
}
void SimtelEventSource::apply_simtel_calibration(ArrayEvent& event)
{
    if(!event.r1) {
        event.r1 = R1Event();
    }
    for (const auto& [tel_id, r0_tel]: event.r0->tels) {
        Eigen::Matrix<double, -1, -1, Eigen::RowMajor> r1_waveform;
        auto gain_selection = Utils::select_gain_channel_by_threshold<uint16_t>(r0_tel->waveform, gain_selector_threshold);
        int n_pixels = r0_tel->waveform[0].rows();
        int n_samples = r0_tel->waveform[0].cols();
        r1_waveform.resize(n_pixels, n_samples);
        for(int ipix = 0; ipix < n_pixels; ipix++) {
            int selected_gain_channel = gain_selection[ipix];
            for(int isample = 0; isample < n_samples; isample++) {
                r1_waveform(ipix, isample) = (r0_tel->waveform[selected_gain_channel](ipix, isample) - event.monitor->tels[tel_id]->pedestal_per_sample(selected_gain_channel, ipix)) * event.monitor->tels[tel_id]->dc_to_pe(selected_gain_channel, ipix);
            }
        }
        event.r1->add_tel(tel_id, n_pixels, n_samples, std::move(r1_waveform), std::move(gain_selection));
    }
}
void SimtelEventSource::read_monitor(ArrayEvent& event)
{
    if(!event.monitor) {
        event.monitor = EventMonitor();
    }
    for(const auto& tel_id: allowed_tels) {
        auto tel_index = simtel_file_handler->tel_id_to_index[tel_id];
        if(!simtel_file_handler->hsdata->event.teldata[tel_index].known)
        {
            continue;
        }
        if(simtel_file_handler->hsdata->tel_lascal[tel_index].known && simtel_file_handler->hsdata->tel_moni[tel_index].known)
        {
            auto n_channels = simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_gains;
            auto n_pixels = simtel_file_handler->hsdata->event.teldata[tel_index].raw->num_pixels;
            Eigen::Matrix<double, -1, -1, Eigen::RowMajor> pedestal_per_sample(n_channels, n_pixels);
            for(int ich = 0; ich < n_channels; ich++) {
                for(int ipix = 0; ipix < n_pixels; ipix++) {
                    pedestal_per_sample(ich, ipix) = simtel_file_handler->hsdata->tel_moni[tel_index].pedsamp[ich][ipix];
                }
            }
            Eigen::Matrix<double, -1, -1, Eigen::RowMajor> dc_to_pe(n_channels, n_pixels);
            for(int ich = 0; ich < n_channels; ich++) {
                for(int ipix = 0; ipix < n_pixels; ipix++) {
                    dc_to_pe(ich, ipix) = simtel_file_handler->hsdata->tel_lascal[tel_index].calib[ich][ipix];
                }
            }
            event.monitor->add_tel(tel_id, n_channels, n_pixels, std::move(pedestal_per_sample), std::move(dc_to_pe));
        }
    }
}
void SimtelEventSource::read_pointing(ArrayEvent& event)
{
    if(!event.pointing) {
        event.pointing = Pointing();
    }
    for(const auto& tel_id: allowed_tels) {
        auto tel_index = simtel_file_handler->tel_id_to_index[tel_id];
        if(simtel_file_handler->hsdata->event.trackdata[tel_index].cor_known)
        {
            // Have corrected pointing information    
            double azimuth = simtel_file_handler->hsdata->event.trackdata[tel_index].azimuth_cor;
            double altitude = simtel_file_handler->hsdata->event.trackdata[tel_index].altitude_cor;
            event.pointing->add_tel(tel_id, azimuth, altitude);
        }
        else if(simtel_file_handler->hsdata->event.trackdata[tel_index].raw_known)
        {
            // Have raw pointing information
            double azimuth = simtel_file_handler->hsdata->event.trackdata[tel_index].azimuth_raw;
            double altitude = simtel_file_handler->hsdata->event.trackdata[tel_index].altitude_raw;
            event.pointing->add_tel(tel_id, azimuth, altitude);
        }
    }
    event.pointing->set_array_pointing(simtel_file_handler->hsdata->run_header.direction[0], simtel_file_handler->hsdata->run_header.direction[1]);
}
void SimtelEventSource::read_true_image(ArrayEvent& event)
{
    for(const auto& tel_id: allowed_tels) {
            auto tel_index = simtel_file_handler->tel_id_to_index[tel_id];
            auto tel_position = subarray->tel_positions[tel_id];
            auto shower_core = std::array<double, 3>{event.simulation->shower.core_x, event.simulation->shower.core_y, 0};
            double cos_alt = cos(event.simulation->shower.alt);
            double sin_alt = sin(event.simulation->shower.alt);
            double cos_az = cos(event.simulation->shower.az);
            double sin_az = sin(event.simulation->shower.az);
            std::array<double, 3> line_direction{
                cos_alt * sin_az,
                cos_alt * cos_az,
                sin_alt
            };
            double impact_parameter = Utils::point_line_distance(tel_position, shower_core, line_direction);
            event.simulation->add_simulated_image(tel_id, simtel_file_handler->hsdata->mc_event.mc_pe_list[tel_index].pixels,simtel_file_handler->hsdata->mc_event.mc_pe_list[tel_index].pe_count, impact_parameter);
        }
}
void SimtelEventSource::read_simulated_showers(ArrayEvent& event)
{
    if(!event.simulation) {
        event.simulation = SimulatedEvent();
    }
    event.run_id = simtel_file_handler->hsdata->run_header.run;
    event.event_id = simtel_file_handler->hsdata->mc_event.event;
    event.simulation->shower.shower_primary_id = simtel_file_handler->hsdata->mc_shower.primary_id;
    event.simulation->shower.energy = simtel_file_handler->hsdata->mc_shower.energy;
    event.simulation->shower.alt = simtel_file_handler->hsdata->mc_shower.altitude;
    event.simulation->shower.az = simtel_file_handler->hsdata->mc_shower.azimuth;
    event.simulation->shower.core_x = simtel_file_handler->hsdata->mc_event.xcore;
    event.simulation->shower.core_y = simtel_file_handler->hsdata->mc_event.ycore;
    event.simulation->shower.h_first_int = simtel_file_handler->hsdata->mc_shower.h_first_int;
    event.simulation->shower.x_max = simtel_file_handler->hsdata->mc_shower.xmax;
    event.simulation->shower.starting_grammage = simtel_file_handler->hsdata->mc_shower.depth_start;
    event.simulation->shower.h_max = simtel_file_handler->hsdata->mc_shower.hmax;
}
const std::string SimtelEventSource::print() const
{
    return spdlog::fmt_lib::format("SimtelEventSource: {}", input_filename);
}