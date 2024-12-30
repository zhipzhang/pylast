#include "nanobind/nanobind.h"
#include "ArrayEvent.hh"
#include "SimulatedShowerArray.hh"
#include "nanobind/eigen/dense.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/optional.h"
#include "nanobind/stl/array.h"
namespace nb = nanobind;

void bind_array_event(nb::module_ &m) {
    nb::class_<ArrayEvent>(m, "ArrayEvent")
        .def_ro("simulation", &ArrayEvent::simulated_event)
        .def_ro("r0_event", &ArrayEvent::r0_event);
    nb::class_<R0Event>(m, "R0Event")
        .def_ro("tels", &R0Event::tels);
    nb::class_<R0Camera>(m, "R0Camera")
        .def_ro("waveform", &R0Camera::waveform)
        .def_ro("waveform_sum", &R0Camera::waveform_sum);
    nb::class_<SimulatedEvent>(m, "SimulatedEvent")
        .def_ro("shower", &SimulatedEvent::shower)
        .def_ro("cameras", &SimulatedEvent::cameras);
    nb::class_<SimulatedCamera>(m, "SimulatedCamera")
        .def_ro("true_image_sum", &SimulatedCamera::true_image_sum)
        .def_ro("true_image", &SimulatedCamera::true_image)
        .def_ro("impact_parameter", &SimulatedCamera::impact_parameter);
    nb::class_<TelImpactParameter>(m, "TelImpactParameter")
        .def_ro("impact_distance", &TelImpactParameter::impact_parameter)
        .def_ro("impact_distance_error", &TelImpactParameter::impact_parameter_error);
    nb::class_<SimulatedShower>(m, "shower")
        .def_ro("alt", &SimulatedShower::alt)
        .def_ro("az", &SimulatedShower::az)
        .def_ro("core_x", &SimulatedShower::core_x)
        .def_ro("core_y", &SimulatedShower::core_y)
        .def_ro("energy", &SimulatedShower::energy)
        .def_ro("h_first_int", &SimulatedShower::h_first_int)
        .def_ro("x_max", &SimulatedShower::x_max)
        .def_ro("starting_grammage", &SimulatedShower::starting_grammage)
        .def_ro("shower_primary_id", &SimulatedShower::shower_primary_id)
        .def("__repr__", &SimulatedShower::print);
}
void bind_simulated_shower_array(nb::module_ &m) {
    nb::class_<SimulatedShowerArray>(m, "SimulatedShowerArray")
        .def_prop_ro("size", &SimulatedShowerArray::size)
        .def_prop_ro("energy", &SimulatedShowerArray::energy)
        .def_prop_ro("alt", &SimulatedShowerArray::alt)
        .def_prop_ro("az", &SimulatedShowerArray::az)
        .def_prop_ro("core_x", &SimulatedShowerArray::core_x)
        .def_prop_ro("core_y", &SimulatedShowerArray::core_y)
        .def_prop_ro("h_first_int", &SimulatedShowerArray::h_first_int)
        .def_prop_ro("x_max", &SimulatedShowerArray::x_max)
        .def_prop_ro("starting_grammage", &SimulatedShowerArray::starting_grammage)
        .def_prop_ro("shower_primary_id", &SimulatedShowerArray::shower_primary_id)
        .def("__repr__", &SimulatedShowerArray::print)
        .def("__getitem__", &SimulatedShowerArray::operator[])
        .def("__len__", &SimulatedShowerArray::size);
}