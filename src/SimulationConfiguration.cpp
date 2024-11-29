#include "SimulationConfiguration.hh"
#include "spdlog/fmt/fmt.h"

const std::string SimulationConfiguration::print() const {
    return fmt::format(
        "SimulationConfiguration(\n"
        "  run_number={},\n"
        "  corsika_version={}, simtel_version={},\n"
        "  energy_range_min={}, energy_range_max={},\n"
        "  prod_site_B_total={},\n"
        "  prod_site_B_declination={}, prod_site_B_inclination={},\n"
        "  prod_site_alt={},\n"
        "  spectral_index={},\n"
        "  shower_prog_start={}, shower_prog_id={},\n"
        "  detector_prog_start={}, detector_prog_id={},\n"
        "  n_showers={}, shower_reuse={},\n"
        "  max_alt={}, min_alt={},\n"
        "  max_az={}, min_az={},\n"
        "  diffuse={},\n"
        "  max_viewcone_radius={}, min_viewcone_radius={},\n"
        "  max_scatter_range={}, min_scatter_range={},\n"
        "  core_pos_mode={},\n"
        "  atmosphere={},\n"
        "  corsika_iact_options={},\n"
        "  corsika_low_E_model={}, corsika_high_E_model={},\n"
        "  corsika_bunchsize={},\n"
        "  corsika_wlen_min={}, corsika_wlen_max={},\n"
        "  corsika_low_E_detail={}, corsika_high_E_detail={}\n"
        ")",
        run_number, corsika_version, simtel_version, energy_range_min, energy_range_max,
        prod_site_B_total, prod_site_B_declination, prod_site_B_inclination, prod_site_alt,
        spectral_index, shower_prog_start, shower_prog_id, detector_prog_start, detector_prog_id,
        n_showers, shower_reuse, max_alt, min_alt, max_az, min_az, diffuse, max_viewcone_radius,
        min_viewcone_radius, max_scatter_range, min_scatter_range, core_pos_mode, atmosphere,
        corsika_iact_options, corsika_low_E_model, corsika_high_E_model, corsika_bunchsize,
        corsika_wlen_min, corsika_wlen_max, corsika_low_E_detail, corsika_high_E_detail
    );
}