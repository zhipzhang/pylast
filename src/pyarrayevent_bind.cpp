#include "R0Event.hh"
#include "nanobind/nanobind.h"
#include "ArrayEvent.hh"
#include "SimulatedShowerArray.hh"
#include "nanobind/eigen/dense.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/optional.h"
#include "nanobind/stl/array.h"
#include "nanobind/stl/unique_ptr.h"
#include "TelMonitor.hh"
#include "EventMonitor.hh"
#include "DL0Event.hh"
#include "DL1Event.hh"
namespace nb = nanobind;

void bind_dl1_event(nb::module_ &m) {
    nb::class_<DL1Event>(m, "DL1Event")
        .def_prop_ro("tels", &DL1Event::get_tels);
    nb::class_<DL1Camera>(m, "DL1Camera")
        .def_ro("image", &DL1Camera::image)
        .def_ro("peak_time", &DL1Camera::peak_time)
        .def_ro("mask", &DL1Camera::mask)
        .def_ro("image_parameters", &DL1Camera::image_parameters);
    nb::class_<ImageParameters>(m, "image_parameters")
        .def_ro("hillas", &ImageParameters::hillas)
        .def_ro("leakage", &ImageParameters::leakage)
        .def_ro("concentration", &ImageParameters::concentration)
        .def_ro("morphology", &ImageParameters::morphology);
    nb::class_<MorphologyParameter>(m, "morphology")
        .def_ro("n_pixels", &MorphologyParameter::n_pixels)
        .def_ro("n_islands", &MorphologyParameter::n_islands)
        .def_ro("n_small_islands", &MorphologyParameter::n_small_islands)
        .def_ro("n_medium_islands", &MorphologyParameter::n_medium_islands)
        .def_ro("n_large_islands", &MorphologyParameter::n_large_islands);
    nb::class_<HillasParameter>(m, "hillas")
        .def_ro("x", &HillasParameter::x)
        .def_ro("y", &HillasParameter::y)
        .def_ro("width", &HillasParameter::width)
        .def_ro("length", &HillasParameter::length)
        .def_ro("phi", &HillasParameter::phi)
        .def_ro("intensity", &HillasParameter::intensity)
        .def_ro("skewness", &HillasParameter::skewness)
        .def_ro("kurtosis", &HillasParameter::kurtosis);
    nb::class_<LeakageParameter>(m, "leakage")
        .def_ro("pixels_width_1", &LeakageParameter::pixels_width_1)
        .def_ro("pixels_width_2", &LeakageParameter::pixels_width_2)
        .def_ro("intensity_width_1", &LeakageParameter::intensity_width_1)
        .def_ro("intensity_width_2", &LeakageParameter::intensity_width_2);
    nb::class_<ConcentrationParameter>(m, "concentration")
        .def_ro("concentration_cog", &ConcentrationParameter::concentration_cog)
        .def_ro("concentration_core", &ConcentrationParameter::concentration_core)
        .def_ro("concentration_pixel", &ConcentrationParameter::concentration_pixel);
}
void bind_dl0_event(nb::module_ &m) {
    nb::class_<DL0Event>(m, "DL0Event")
        .def_prop_ro("tels", &DL0Event::get_tels);
    nb::class_<DL0Camera>(m, "DL0Camera")
        .def_ro("image", &DL0Camera::image)
        .def_ro("peak_time", &DL0Camera::peak_time);
}
void bind_r1_event(nb::module_ &m) {
    nb::class_<R1Event>(m, "R1Event")
        .def_prop_ro("tels", &R1Event::get_tels);
    nb::class_<R1Camera>(m, "R1Camera")
        .def_ro("waveform", &R1Camera::waveform)
        .def_ro("gain_selection", &R1Camera::gain_selection);
}
void bind_r0_event(nb::module_ &m) {
    nb::class_<R0Event>(m, "R0Event")
        .def_prop_ro("tels", &R0Event::get_tels);
    nb::class_<R0Camera>(m, "R0Camera")
        .def_ro("waveform", &R0Camera::waveform)
        .def_ro("waveform_sum", &R0Camera::waveform_sum);
}
void bind_simulated_event(nb::module_ &m) {
    nb::class_<SimulatedEvent>(m, "SimulatedEvent")
        .def_ro("shower", &SimulatedEvent::shower)
        .def_prop_ro("tels", &SimulatedEvent::get_tels);
    nb::class_<SimulatedCamera>(m, "SimulatedCamera")
        .def_ro("true_image_sum", &SimulatedCamera::true_image_sum)
        .def_ro("true_image", &SimulatedCamera::true_image)
        .def_ro("impact", &SimulatedCamera::impact)
        .def("__repr__", &SimulatedCamera::print);
    nb::class_<TelImpactParameter>(m, "TelImpactParameter")
        .def_ro("impact_distance", &TelImpactParameter::distance)
        .def_ro("impact_distance_error", &TelImpactParameter::distance_error);
    nb::class_<SimulatedShower>(m, "SimulatedShower")
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

void bind_tel_monitor(nb::module_ &m) {
    nb::class_<TelMonitor>(m, "TelMonitor")
        .def_ro("n_channels", &TelMonitor::n_channels)
        .def_ro("n_pixels", &TelMonitor::n_pixels)
        .def_ro("pedestal_per_sample", &TelMonitor::pedestal_per_sample)
        .def_ro("dc_to_pe", &TelMonitor::dc_to_pe);
}
void bind_array_event(nb::module_ &m) {
    nb::class_<ArrayEvent>(m, "ArrayEvent")
        .def_ro("simulation", &ArrayEvent::simulation)
        .def_ro("r0", &ArrayEvent::r0)
        .def_ro("monitor", &ArrayEvent::monitor)
        .def_ro("r1", &ArrayEvent::r1)
        .def_ro("dl0", &ArrayEvent::dl0)
        .def_ro("dl1", &ArrayEvent::dl1);  // Added missing dl1 binding
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
NB_MODULE(_pylast_arrayevent, m) {
    bind_dl0_event(m);
    bind_dl1_event(m);
    bind_r0_event(m);
    bind_r1_event(m);
    bind_simulated_event(m);
    bind_tel_monitor(m);
    bind_array_event(m);
    bind_simulated_shower_array(m);
}