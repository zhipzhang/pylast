#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/pair.h"
#include "nanobind/make_iterator.h"
#include "nanobind/stl/optional.h"
#include "ShowerProcessor.hh"
namespace nb = nanobind;

void bind_showerprocessor(nb::module_ &m) {
    nb::class_<ShowerProcessor>(m, "ShowerProcessor")
        .def(nb::init<const SubarrayDescription&>(), nb::arg("subarray"))
        .def (nb::init<const SubarrayDescription&, const std::string&>(), nb::arg("subarray"), nb::arg("config"))
        .def("__call__", [](ShowerProcessor& self, ArrayEvent& event) {
            self(event);
        })
        .def("__repr__", [](ShowerProcessor& self) {
            return "ShowerProcessor:\n  Config: " + self.get_config_str();
        });
}

NB_MODULE(_pylast_showerprocessor, m) {
    bind_showerprocessor(m);
}


