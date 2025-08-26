#include "DataWriter.hh"
#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/pair.h"
#include "SubarrayDescription.hh"
#include "nanobind/stl/unique_ptr.h"
#include <iostream>
#include "nanobind/stl/string.h"
namespace nb = nanobind;

void bind_datawriter(nb::module_ &m)
{
    nb::class_<DataWriter>(m, "DataWriter")
        .def(nb::init< EventSource&, const std::string&>(), nb::arg("source"), nb::arg("filename"), nb::rv_policy::reference_internal)
        .def(nb::init< EventSource&, const std::string&, const std::string&>(), nb::arg("source"), nb::arg("filename"), nb::arg("config_str"))
        .def("__call__", [](DataWriter& self, ArrayEvent& event) {
            self(event);
        })
        .def("__repr__", [](DataWriter& self) {
            return "DataWriter:\n  Config: " + self.get_config_str();
        })
        .def("close", &DataWriter::close)
        .def("write_all_simulation_shower", &DataWriter::write_all_simulation_shower)
        .def("write_statistics", &DataWriter::write_statistics, nb::arg("statistics"), nb::arg("last") = false);
}

NB_MODULE(_pylast_datawriter, m){
    bind_datawriter(m);
}