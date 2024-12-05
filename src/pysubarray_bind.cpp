#include "nanobind/nanobind.h"
#include "SubarrayDescription.hh"
#include "nanobind/eigen/dense.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/array.h"
#include "nanobind/stl/string.h"
namespace nb = nanobind;

void bind_subarray_description(nb::module_ &m)
{
    nb::class_<SubarrayDescription>(m, "SubarrayDescription")
        .def_ro("tels", &SubarrayDescription::tel_descriptions)
        .def_ro("tel_positions", &SubarrayDescription::tel_positions)
        .def("__repr__", &SubarrayDescription::print);
    nb::class_<TelescopeDescription>(m, "TelescopeDescription")
        .def_ro("camera", &TelescopeDescription::camera_description)
        .def_ro("optics", &TelescopeDescription::optics_description)
        .def("__repr__", &TelescopeDescription::print);
    nb::class_<OpticsDescription>(m, "OpticsDescription")
        .def_ro("optics_name", &OpticsDescription::optics_name)
        .def_ro("num_mirrors", &OpticsDescription::num_mirrors)
        .def_ro("mirror_area", &OpticsDescription::mirror_area)
        .def_ro("equivalent_focal_length", &OpticsDescription::equivalent_focal_length)
        .def_ro("effective_focal_length", &OpticsDescription::effective_focal_length)
        .def("__repr__", &OpticsDescription::print);

    nb::class_<CameraDescription>(m, "CameraDescription")
        .def_ro("camera_name", &CameraDescription::camera_name)
        .def_ro("geometry", &CameraDescription::camera_geometry)
        .def_ro("readout", &CameraDescription::camera_readout)
        .def("__repr__", &CameraDescription::print);
    nb::class_<CameraReadout>(m, "CameraReadout")
        .def_ro("camera_name", &CameraReadout::camera_name)
        .def_ro("sampling_rate", &CameraReadout::sampling_rate)
        .def_ro("reference_pulse_shape", &CameraReadout::reference_pulse_shape)
        .def_ro("reference_pulse_sample_width", &CameraReadout::reference_pulse_sample_width)
        .def_ro("n_channels", &CameraReadout::n_channels)
        .def_ro("n_pixels", &CameraReadout::n_pixels)
        .def_ro("n_samples", &CameraReadout::n_samples)
        .def("__repr__", &CameraReadout::print);
    nb::class_<CameraGeometry>(m, "CameraGeometry")
        .def_ro("camera_name", &CameraGeometry::camera_name)
        .def_ro("pix_type", &CameraGeometry::pix_type)
        .def_ro("pix_x", &CameraGeometry::pix_x)
        .def_ro("pix_y", &CameraGeometry::pix_y)
        .def_ro("pix_area", &CameraGeometry::pix_area)
        .def_ro("cam_rotation", &CameraGeometry::cam_rotation)
        .def("__repr__", &CameraGeometry::print);
}
