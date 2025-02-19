#include "EventSource.hh"
#include "SimulationConfiguration.hh"
#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/pair.h"
#include "nanobind/make_iterator.h"
#include "nanobind/stl/optional.h"
#include "LoggerInitialize.hh"
namespace nb = nanobind;

NB_MODULE(_pyeventsource, m){
    nb::class_<EventSource>(m, "EventSource")
        .def_ro("input_filename", &EventSource::input_filename)
        .def_ro("is_stream", &EventSource::is_stream)
        .def_ro("max_events", &EventSource::max_events)
        .def_ro("allowed_tels", &EventSource::allowed_tels)
        .def_ro("simulation_config", &EventSource::simulation_config)
        .def_ro("atmosphere_model", &EventSource::atmosphere_model)
        .def_ro("metaparam", &EventSource::metaparam)
        .def_ro("subarray", &EventSource::subarray)
        .def_ro("shower_array", &EventSource::shower_array)
        .def("load_simulated_showers", &EventSource::load_all_simulated_showers)
        .def("__iter__",
            [](EventSource &source) {
                return nb::make_iterator<nb::rv_policy::move>(
                    nb::type<EventSource>(),
                    "EventSourceIterator",
                    source.begin(),
                    source.end()
                );
            });
    nb::class_<SimulationConfiguration>(m, "SimulationConfiguration")
        .def(nb::init<>())
        .def_ro("run_number", &SimulationConfiguration::run_number)
        .def_ro("corsika_version", &SimulationConfiguration::corsika_version)
        .def_ro("simtel_version", &SimulationConfiguration::simtel_version)
        .def_ro("energy_range_min", &SimulationConfiguration::energy_range_min)
        .def_ro("energy_range_max", &SimulationConfiguration::energy_range_max)
        .def_ro("prod_site_B_total", &SimulationConfiguration::prod_site_B_total)
        .def_ro("prod_site_B_declination", &SimulationConfiguration::prod_site_B_declination)
        .def_ro("prod_site_B_inclination", &SimulationConfiguration::prod_site_B_inclination)
        .def_ro("prod_site_alt", &SimulationConfiguration::prod_site_alt)
        .def_ro("spectral_index", &SimulationConfiguration::spectral_index)
        .def_ro("shower_prog_start", &SimulationConfiguration::shower_prog_start)
        .def_ro("shower_prog_id", &SimulationConfiguration::shower_prog_id)
        .def_ro("detector_prog_start", &SimulationConfiguration::detector_prog_start)
        .def_ro("detector_prog_id", &SimulationConfiguration::detector_prog_id)
        .def_ro("n_showers", &SimulationConfiguration::n_showers)
        .def_ro("shower_reuse", &SimulationConfiguration::shower_reuse)
        .def_ro("max_alt", &SimulationConfiguration::max_alt)
        .def_ro("min_alt", &SimulationConfiguration::min_alt)
        .def_ro("max_az", &SimulationConfiguration::max_az)
        .def_ro("min_az", &SimulationConfiguration::min_az)
        .def_ro("diffuse", &SimulationConfiguration::diffuse)
        .def_ro("max_viewcone_radius", &SimulationConfiguration::max_viewcone_radius)
        .def_ro("min_viewcone_radius", &SimulationConfiguration::min_viewcone_radius)
        .def_ro("max_scatter_range", &SimulationConfiguration::max_scatter_range)
        .def_ro("min_scatter_range", &SimulationConfiguration::min_scatter_range)
        .def_ro("core_pos_mode", &SimulationConfiguration::core_pos_mode)
        .def_ro("atmosphere", &SimulationConfiguration::atmosphere)
        .def_ro("corsika_iact_options", &SimulationConfiguration::corsika_iact_options)
        .def_ro("corsika_low_E_model", &SimulationConfiguration::corsika_low_E_model)
        .def_ro("corsika_high_E_model", &SimulationConfiguration::corsika_high_E_model)
        .def_ro("corsika_bunchsize", &SimulationConfiguration::corsika_bunchsize)
        .def_ro("corsika_wlen_min", &SimulationConfiguration::corsika_wlen_min)
        .def_ro("corsika_wlen_max", &SimulationConfiguration::corsika_wlen_max)
        .def_ro("corsika_low_E_detail", &SimulationConfiguration::corsika_low_E_detail)
        .def_ro("corsika_high_E_detail", &SimulationConfiguration::corsika_high_E_detail)
        .def("__repr__", &SimulationConfiguration::print);
    nb::class_<TableAtmosphereModel>(m, "TableAtmosphereModel")
        .def(nb::init<const std::string&>())
        .def(nb::init<int, double*, double*, double*, double*>())
        .def_ro("input_filename", &TableAtmosphereModel::input_filename)
        .def_ro("n_alt", &TableAtmosphereModel::n_alt)
        .def_ro("alt_km", &TableAtmosphereModel::alt_km)
        .def_ro("rho", &TableAtmosphereModel::rho)
        .def_ro("thick", &TableAtmosphereModel::thick)
        .def_ro("refidx_m1", &TableAtmosphereModel::refidx_m1)
        .def("__repr__", &TableAtmosphereModel::print);
    nb::class_<Metaparam>(m, "Metaparam")
        .def_ro("global_metadata", &Metaparam::global_metadata)
        .def_ro("tel_metadata", &Metaparam::tel_metadata)
        .def_ro("history", &Metaparam::history)
        .def_ro("tel_history", &Metaparam::tel_history);
    m.def("initialize_logger", &initialize_logger,
          nb::arg("log_level") = "info", 
          nb::arg("log_file") = "");
}