#include "Calibration.hh"
#include "ImageExtractor.hh"
#include "LACT1Calibrator.hh"
#include "SubarrayDescription.hh"
#include "nanobind/nanobind.h"
#include "nanobind/stl/pair.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/unique_ptr.h"
#include "nanobind/stl/vector.h"
#include <iostream>
namespace nb = nanobind;

void bind_calibrator(nb::module_ &m) {
  nb::class_<Calibrator>(m, "Calibrator")
      .def(nb::init<const SubarrayDescription &, const std::string &>(),
           nb::arg("subarray"), nb::arg("config_str"))
      .def(nb::init<const SubarrayDescription &>(), nb::arg("subarray"))
      .def("__call__", [](Calibrator &self, ArrayEvent &event) { self(event); })
      .def("__repr__", [](Calibrator &self) {
        return "Calibrator:\n  Config: " + self.get_config_str();
      });
  nb::class_<LACT1Calibrator>(m, "LACT1Calibrator")
      .def(nb::init<>())
      .def(nb::init<const std::string &>(), nb::arg("config_str"))
      .def("__call__",
           [](LACT1Calibrator &self, ArrayEvent &event) { self(event); })
      .def("__repr__", [](LACT1Calibrator &self) {
        return "LACT1Calibrator:\n  Config: " + self.get_config_str();
      });
}

NB_MODULE(_pylast_calibrator, m) { bind_calibrator(m); }