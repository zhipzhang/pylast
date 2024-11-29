#include "SimtelEventSource.hh"
#include "LACT_hessioxxx/include/io_basic.h"
#include "LACT_hessioxxx/include/io_hess.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/vector.h"
#include <cassert>
const std::string ihep_url = "root://eos01.ihep.ac.cn/";
SimtelEventSource::SimtelEventSource(const string& filename, int64_t max_events, std::vector<int> subarray) : EventSource(filename, max_events, subarray)
{
    if ( (iobuf = allocate_io_buffer(5000000L)) == NULL )
   {
      spdlog::error("Cannot allocate I/O buffer");
      throw std::runtime_error("Cannot allocate I/O buffer");
   }
    iobuf->output_file = NULL;
    // for 1GB file at least.
    iobuf->max_length  = 1000000000L; 
    open_file(input_filename);
    iobuf->input_file = input_file;
    init_simulation_config();
    
}

void SimtelEventSource::open_file(const string& filename)
{
    if (filename.substr(0, 4) == "/eos") {
        // If filename starts with "eos", prepend the IHEP URL
        string full_path = ihep_url  + filename;
        spdlog::info("Opening EOS file: {}", full_path);
        input_file = fileopen(full_path.c_str(), "rb");
        if(input_file == NULL)
        {
            spdlog::error("Failed to open EOS file: {}", full_path);
            throw std::runtime_error(spdlog::fmt_lib::format("Failed to open EOS file: {}", full_path));
        }
        is_stream = true;
        return;
    }
    else
    {
        input_file = fileopen(filename.c_str(), "rb");
        if(input_file == NULL)
        {
            spdlog::error("Failed to open file: {}", filename);
            throw std::runtime_error(spdlog::fmt_lib::format("Failed to open local file: {}", filename));
        }
        is_stream = true; // Cause for simtel file, it's always be compressed, so it's a stream.
    }
}
void SimtelEventSource::read_block()
{
    if(find_io_block(iobuf, &item_header)!=0)
    {
        spdlog::error("Failed to find IO block");
        throw std::runtime_error("Failed to find IO block");
    }
    if(read_io_block(iobuf, &item_header)!=0)
    {
        spdlog::error("Failed to read IO block");
        throw std::runtime_error("Failed to read IO block");
    }
}
void SimtelEventSource::read_runheader()
{
    assert(item_header.type == IO_TYPE_SIMTEL_RUNHEADER);
    hsdata = (AllHessData* ) calloc(1, sizeof(AllHessData));
    if(hsdata == NULL)
    {
        spdlog::error("Failed to allocate memory for hsdata");
        throw std::runtime_error("Failed to allocate memory for hsdata");
    }
    if(read_simtel_runheader(iobuf, &hsdata->run_header) < 0)
    {
        spdlog::error("Failed to read runheader");
        throw std::runtime_error("Failed to read runheader");
    }
    for(auto itel = 0; itel < hsdata->run_header.ntel; itel++)
    {
        if(!is_subarray_selected(hsdata->run_header.tel_id[itel])) continue;
        int tel_id = hsdata->run_header.tel_id[itel];
        spdlog::info("Initialize telescope id: {}", tel_id);
        tel_id_to_index[tel_id] = itel;
        hsdata->camera_set[itel].tel_id = tel_id;
        hsdata->camera_org[itel].tel_id = tel_id;
        hsdata->pixel_disabled[itel].tel_id = tel_id;
        hsdata->pixel_set[itel].tel_id  = tel_id;
        hsdata->cam_soft_set[itel].tel_id = tel_id;
        hsdata->tracking_set[itel].tel_id = tel_id;
        hsdata->point_cor[itel].tel_id = tel_id;
        hsdata->event.num_tel++;
        hsdata->event.teldata[itel].tel_id = tel_id;
        hsdata->event.trackdata[itel].tel_id = tel_id;
        hsdata->event.teldata[itel].raw = (AdcData*) calloc(1, sizeof(AdcData));
        if(hsdata->event.teldata[itel].raw == NULL)
        {
            spdlog::error("Failed to allocate memory for raw adc data");
            throw std::runtime_error("Failed to allocate memory for raw adc data");
        }
        hsdata->event.teldata[itel].raw->tel_id = tel_id;
        hsdata->event.teldata[itel].pixtm = (PixelTiming*) calloc(1, sizeof(PixelTiming));
        if(hsdata->event.teldata[itel].pixtm == NULL)
        {
            spdlog::error("Failed to allocate memory for pixel timing data");
            throw std::runtime_error("Failed to allocate memory for pixel timing data");
        }
        hsdata->event.teldata[itel].pixtm->tel_id = tel_id;
        hsdata->event.teldata[itel].img = (ImgData*) calloc(1, sizeof(ImgData));
        if(hsdata->event.teldata[itel].img == NULL)
        {
            spdlog::error("Failed to allocate memory for image data");
            throw std::runtime_error("Failed to allocate memory for image data");
        }
        hsdata->event.teldata[itel].img->tel_id = tel_id;
        hsdata->event.teldata[itel].max_image_sets = 0;
        hsdata->event.teldata[itel].img[0].tel_id = tel_id;
        hsdata->event.teldata[itel].img[1].tel_id = tel_id;
        hsdata->tel_moni[itel].tel_id = tel_id;
        hsdata->tel_lascal[itel].tel_id = tel_id;

    }

}
void SimtelEventSource::read_mcrunheader()
{
    assert(item_header.type == IO_TYPE_SIMTEL_MCRUNHEADER);
    if(read_simtel_mcrunheader(iobuf, &hsdata->mc_run_header) < 0)
    {
        spdlog::error("Failed to read mcrunheader");
        throw std::runtime_error("Failed to read mcrunheader");
    }
}
void SimtelEventSource::init_simulation_config()
{
    while(item_header.type != IO_TYPE_SIMTEL_RUNHEADER)
    {
        read_block();
        spdlog::debug("Read runheader block, block type: {}", item_header.type);
    }
    read_runheader();
    spdlog::info("Read runheader block, block type: {}", item_header.type);
    while(item_header.type != IO_TYPE_SIMTEL_MCRUNHEADER)
    {
        read_block();
    }
    read_mcrunheader();
    set_simulation_config();
}

void SimtelEventSource::set_simulation_config()
{
    spdlog::info("Set simulation config");
    simulation_config.run_number = hsdata->run_header.run;
    simulation_config.corsika_version = hsdata->mc_run_header.shower_prog_vers;
    simulation_config.simtel_version = hsdata->mc_run_header.detector_prog_vers;
    simulation_config.energy_range_min = hsdata->mc_run_header.E_range[0];
    spdlog::info("Energy range min: {}", simulation_config.energy_range_min);
    simulation_config.energy_range_max = hsdata->mc_run_header.E_range[1];
    simulation_config.prod_site_B_total = hsdata->mc_run_header.B_total;
    simulation_config.prod_site_B_declination = hsdata->mc_run_header.B_declination;
    simulation_config.prod_site_B_inclination = hsdata->mc_run_header.B_inclination;
    simulation_config.prod_site_alt = hsdata->mc_run_header.obsheight;
    simulation_config.spectral_index = hsdata->mc_run_header.spectral_index;
    simulation_config.shower_prog_start = hsdata->mc_run_header.shower_prog_start;
    simulation_config.shower_prog_id = hsdata->mc_run_header.shower_prog_id;
    simulation_config.detector_prog_start = hsdata->mc_run_header.detector_prog_start;
    simulation_config.n_showers = hsdata->mc_run_header.num_showers;
    simulation_config.shower_reuse = hsdata->mc_run_header.num_use;
    simulation_config.max_alt = hsdata->mc_run_header.alt_range[1];
    simulation_config.min_alt = hsdata->mc_run_header.alt_range[0];
    simulation_config.max_az = hsdata->mc_run_header.az_range[1];
    simulation_config.min_az = hsdata->mc_run_header.az_range[0];
    simulation_config.diffuse = hsdata->mc_run_header.diffuse;
    simulation_config.max_viewcone_radius = hsdata->mc_run_header.viewcone[1];
    simulation_config.min_viewcone_radius = hsdata->mc_run_header.viewcone[0];
    simulation_config.atmosphere = hsdata->mc_run_header.atmosphere;
    simulation_config.corsika_iact_options = hsdata->mc_run_header.corsika_iact_options;
    simulation_config.corsika_bunchsize = static_cast<int>(hsdata->mc_run_header.corsika_bunchsize);
    simulation_config.corsika_low_E_model = hsdata->mc_run_header.corsika_low_E_model;
    simulation_config.corsika_high_E_model = hsdata->mc_run_header.corsika_high_E_model;
    simulation_config.corsika_wlen_min = hsdata->mc_run_header.corsika_wlen_min;
    simulation_config.corsika_wlen_max = hsdata->mc_run_header.corsika_wlen_max;
}
const std::string SimtelEventSource::print() const
{
    return spdlog::fmt_lib::format("SimtelEventSource: {}", input_filename);
}

void SimtelEventSource::bind(nb::module_& m)
{
    nb::class_<SimtelEventSource, EventSource>(m, "SimtelEventSource")
        .def(nb::init<const std::string&, int64_t, std::vector<int>>(), nb::arg("filename"), nb::arg("max_events") = -1, nb::arg("subarray")=std::vector<int>{})
        .def("__repr__", &SimtelEventSource::print);
}
