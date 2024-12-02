#include "SimtelEventSource.hh"
#include "LACT_hessioxxx/include/io_basic.h"
#include "LACT_hessioxxx/include/io_hess.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"
#include <cassert>
#include "LACT_hessioxxx/include/io_history.h"
#include "LACT_hessioxxx/include/mc_tel.h"
#include "LACT_hessioxxx/include/mc_atmprof.h"

SimtelEventSource::SimtelEventSource(const std::string& filename, int64_t max_events, std::vector<int> subarray):
    EventSource(filename, max_events, subarray)
{
    simtel_file_handler = std::make_unique<SimtelFileHandler>(filename, subarray);
    is_stream = true;
    //read_history();
    init_simulation_config();
    init_atmosphere_model();
}


void SimtelEventSource::init_atmosphere_model()
{
    atmosphere_model = TableAtmosphereModel(simtel_file_handler->atmprof->n_alt, simtel_file_handler->atmprof->alt_km, simtel_file_handler->atmprof->rho, simtel_file_handler->atmprof->thick, simtel_file_handler->atmprof->refidx_m1);
}
void SimtelEventSource::init_simulation_config()
{
    set_simulation_config();
}

void SimtelEventSource::set_simulation_config()
{
    simulation_config.run_number = simtel_file_handler->hsdata->run_header.run;
    simulation_config.corsika_version = simtel_file_handler->hsdata->mc_run_header.shower_prog_vers;
    simulation_config.simtel_version = simtel_file_handler->hsdata->mc_run_header.detector_prog_vers;
    simulation_config.energy_range_min = simtel_file_handler->hsdata->mc_run_header.E_range[0];
    simulation_config.energy_range_max = simtel_file_handler->hsdata->mc_run_header.E_range[1];
    simulation_config.prod_site_B_total = simtel_file_handler->hsdata->mc_run_header.B_total;
    simulation_config.prod_site_B_declination = simtel_file_handler->hsdata->mc_run_header.B_declination;
    simulation_config.prod_site_B_inclination = simtel_file_handler->hsdata->mc_run_header.B_inclination;
    simulation_config.prod_site_alt = simtel_file_handler->hsdata->mc_run_header.obsheight;
    simulation_config.spectral_index = simtel_file_handler->hsdata->mc_run_header.spectral_index;
    simulation_config.shower_prog_start = simtel_file_handler->hsdata->mc_run_header.shower_prog_start;
    simulation_config.shower_prog_id = simtel_file_handler->hsdata->mc_run_header.shower_prog_id;
    simulation_config.detector_prog_start = simtel_file_handler->hsdata->mc_run_header.detector_prog_start;
    simulation_config.n_showers = simtel_file_handler->hsdata->mc_run_header.num_showers;
    simulation_config.shower_reuse = simtel_file_handler->hsdata->mc_run_header.num_use;
    simulation_config.max_alt = simtel_file_handler->hsdata->mc_run_header.alt_range[1];
    simulation_config.min_alt = simtel_file_handler->hsdata->mc_run_header.alt_range[0];
    simulation_config.max_az = simtel_file_handler->hsdata->mc_run_header.az_range[1];
    simulation_config.min_az = simtel_file_handler->hsdata->mc_run_header.az_range[0];
    simulation_config.diffuse = simtel_file_handler->hsdata->mc_run_header.diffuse;
    simulation_config.max_viewcone_radius = simtel_file_handler->hsdata->mc_run_header.viewcone[1];
    simulation_config.min_viewcone_radius = simtel_file_handler->hsdata->mc_run_header.viewcone[0];
    simulation_config.atmosphere = simtel_file_handler->hsdata->mc_run_header.atmosphere;
    simulation_config.corsika_iact_options = simtel_file_handler->hsdata->mc_run_header.corsika_iact_options;
    simulation_config.corsika_bunchsize = static_cast<int>(simtel_file_handler->hsdata->mc_run_header.corsika_bunchsize);
    simulation_config.corsika_low_E_model = simtel_file_handler->hsdata->mc_run_header.corsika_low_E_model;
    simulation_config.corsika_high_E_model = simtel_file_handler->hsdata->mc_run_header.corsika_high_E_model;
    simulation_config.corsika_wlen_min = simtel_file_handler->hsdata->mc_run_header.corsika_wlen_min;
    simulation_config.corsika_wlen_max = simtel_file_handler->hsdata->mc_run_header.corsika_wlen_max;
}
const std::string SimtelEventSource::print() const
{
    return spdlog::fmt_lib::format("SimtelEventSource: {}", input_filename);
}

