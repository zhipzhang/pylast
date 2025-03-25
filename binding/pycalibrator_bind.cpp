#include "Calibration.hh"
#include "ImageExtractor.hh"
#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/pair.h"
#include "SubarrayDescription.hh"
#include "nanobind/stl/unique_ptr.h"
#include <iostream>
#include "nanobind/stl/string.h"
namespace nb = nanobind;

void bind_calibrator(nb::module_ &m)
{
    nb::class_<Calibrator>(m, "Calibrator")
        .def(nb::init<const SubarrayDescription&, const std::string&>(), nb::arg("subarray"), nb::arg("config_str") )
        .def(nb::init<const SubarrayDescription&>(), nb::arg("subarray"))
        .def("__call__", [](Calibrator& self, ArrayEvent& event) {
            self(event);
        })
        .def("__repr__", [](Calibrator& self) {
            return "Calibrator:\n  Config: " + self.get_config_str();
        });
}

NB_MODULE(_pylast_calibrator, m){
    bind_calibrator(m);
}