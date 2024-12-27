#include "SimtelEventSource.hh"
#include "CameraDescription.hh"
#include "CameraGeometry.hh"
#include "Eigen/src/Core/util/Constants.h"
#include "Eigen/src/Core/util/Macros.h"
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
#include <thread>
#include "Utils.hh"
SimtelEventSource::SimtelEventSource(const std::string& filename, int64_t max_events, std::vector<int> subarray, bool load_simulated_showers):
    EventSource(filename, max_events, subarray, load_simulated_showers)
{
    simtel_file_handler = std::make_unique<SimtelFileHandler>(filename);
    is_stream = true;
    simtel_file_handler->read_until_event();
    init_metaparam();
    init_simulation_config();
    init_atmosphere_model();
    init_subarray();
    if(load_simulated_showers){
        load_all_simulated_showers();
    }
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
        for(auto itel = 0; itel < simtel_file_handler->history_container.ntel; itel++) {
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
    double* reference_pulse_shape = &simtel_file_handler->hsdata->pixel_set[tel_index].refshape[0][0];
    int n_ref_shape = simtel_file_handler->hsdata->pixel_set[tel_index].nrefshape;
    int l_ref_shape = simtel_file_handler->hsdata->pixel_set[tel_index].lrefshape;
    return CameraReadout(camera_name, sampling_rate, reference_pulse_sample_width, n_channels, num_pixels, n_samples, reference_pulse_shape, n_ref_shape, l_ref_shape   );
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
            temp_shower_array.push_back(shower);
        }
        
        // Copy the temporary shower array to the main shower array
        shower_array = std::move(temp_shower_array);
    };

    // Launch the loading in a separate thread
    std::thread loader_thread(load_showers);
    loader_thread.join();
}
void SimtelEventSource::_load_next_events()
{
    simtel_file_handler->load_next_event();
}
ArrayEvent SimtelEventSource::get_event()
{
    _load_next_events();
    ArrayEvent event;
    event.simulated_event.shower.shower_primary_id = simtel_file_handler->hsdata->mc_shower.primary_id;
    event.simulated_event.shower.energy = simtel_file_handler->hsdata->mc_shower.energy;
    event.simulated_event.shower.alt = simtel_file_handler->hsdata->mc_shower.altitude;
    event.simulated_event.shower.az = simtel_file_handler->hsdata->mc_shower.azimuth;
    event.simulated_event.shower.core_x = simtel_file_handler->hsdata->mc_event.xcore;
    event.simulated_event.shower.core_y = simtel_file_handler->hsdata->mc_event.ycore;
    event.simulated_event.shower.h_first_int = simtel_file_handler->hsdata->mc_shower.h_first_int;
    event.simulated_event.shower.x_max = simtel_file_handler->hsdata->mc_shower.xmax;
    event.simulated_event.shower.starting_grammage = simtel_file_handler->hsdata->mc_shower.depth_start;
    if(simtel_file_handler->have_true_image)
    {
        for(const auto& tel_id: allowed_tels) {
            auto tel_index = simtel_file_handler->tel_id_to_index[tel_id];
            auto tel_position = subarray->tel_positions[tel_id];
            auto shower_core = std::array<double, 3>{event.simulated_event.shower.core_x, event.simulated_event.shower.core_y, 0};
            double cos_alt = cos(event.simulated_event.shower.alt);
            double sin_alt = sin(event.simulated_event.shower.alt);
            double cos_az = cos(event.simulated_event.shower.az);
            double sin_az = sin(event.simulated_event.shower.az);
            std::array<double, 3> line_direction{
                cos_alt * sin_az,
                cos_alt * cos_az,
                sin_alt
            };
            double impact_parameter = Utils::point_line_distance(tel_position, shower_core, line_direction);
            event.add_simulated_camera_image(tel_id, simtel_file_handler->hsdata->mc_event.mc_pe_list[tel_index].pixels,simtel_file_handler->hsdata->mc_event.mc_pe_list[tel_index].pe_count, impact_parameter);
        }
    }
    return event;
}
const std::string SimtelEventSource::print() const
{
    return spdlog::fmt_lib::format("SimtelEventSource: {}", input_filename);
}