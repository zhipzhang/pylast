#include "EventSource.hh"
#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/pair.h"
#include "nanobind/make_iterator.h"
#include "nanobind/stl/optional.h"
#include "SimtelEventSource.hh"
namespace nb = nanobind;

NB_MODULE(_pysimtelsource, m){
    nb::class_<SimtelEventSource, EventSource>(m, "SimtelEventSource")
        .def(nb::init<const std::string&, int64_t, std::vector<int>, bool>(), nb::arg("filename"), nb::arg("max_events") = -1, nb::arg("subarray")=std::vector<int>{}, nb::arg("load_simulated_showers")=false)
        .def("__repr__", &SimtelEventSource::print);
}