#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/pair.h"
#include "nanobind/make_iterator.h"
#include "nanobind/stl/optional.h"
#include "ImageProcessor.hh"

namespace nb = nanobind;

void bind_imageprocessor(nb::module_ &m)
{
    nb::class_<ImageProcessor>(m, "ImageProcessor")
        .def(nb::init<const SubarrayDescription&, const std::string&>(), 
             nb::arg("subarray"), 
             nb::arg("config_str"))
        .def(nb::init<const SubarrayDescription&>(), 
             nb::arg("subarray"))
        .def("__call__", [](ImageProcessor& self, ArrayEvent& event) {
            self(event);
        });
}

NB_MODULE(_pylast_imageprocessor, m){
    bind_imageprocessor(m);
}