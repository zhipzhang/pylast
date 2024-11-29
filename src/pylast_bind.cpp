#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"
#include "EventSource.hh"
#include "SimtelEventSource.hh"
#include "LoggerInitialize.hh"
namespace nb = nanobind;


NB_MODULE(_pylast, m) {
    EventSource::bind(m);
    SimtelEventSource::bind(m);
    m.def("initialize_logger", &initialize_logger,
          nb::arg("log_level") = "info", 
          nb::arg("log_file") = "",
          "Initialize the spdlog logger with specified log level. Optionally specify a log file.");
    SimulationConfiguration::bind(m);
}
