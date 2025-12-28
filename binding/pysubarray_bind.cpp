#include "nanobind/nanobind.h"
#include "SubarrayDescription.hh"
#include "nanobind/eigen/dense.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/array.h"
#include "nanobind/stl/string.h"
#include "nanobind/eigen/sparse.h"
namespace nb = nanobind;

void bind_subarray_description(nb::module_ &m)
{
    nb::class_<SubarrayDescription>(m, "SubarrayDescription")
        .def_ro("tels", &SubarrayDescription::tels)
        .def_ro("tel_positions", &SubarrayDescription::tel_positions)
        .def("__repr__", &SubarrayDescription::print)
        .def_prop_ro("get_pix_x", [](const SubarrayDescription& self)->Eigen::VectorXd{
            auto id_vecs = self.get_ordered_telescope_ids();
            auto pix_x = self.tels.at(id_vecs[0]).camera_description.camera_geometry.pix_x;
            double focal_length = self.tels.at(id_vecs[0]).optics_description.effective_focal_length;
            return pix_x.array()/focal_length * 180/M_PI;
        })
        .def_prop_ro("get_pix_y", [](const SubarrayDescription& self)->Eigen::VectorXd{
            auto id_vecs = self.get_ordered_telescope_ids();
            auto pix_y = self.tels.at(id_vecs[0]).camera_description.camera_geometry.pix_y;
            double focal_length = self.tels.at(id_vecs[0]).optics_description.effective_focal_length;
            return pix_y.array()/focal_length * 180/M_PI;
        })
        .def_prop_ro("get_pix_x_equivalent", [](const SubarrayDescription& self)->Eigen::VectorXd{
            auto id_vecs = self.get_ordered_telescope_ids();
            auto pix_x = self.tels.at(id_vecs[0]).camera_description.camera_geometry.pix_x;
            double focal_length = self.tels.at(id_vecs[0]).optics_description.equivalent_focal_length;
            return pix_x.array()/focal_length * 180/M_PI;
        })
        .def_prop_ro("get_pix_y_equivalent", [](const SubarrayDescription& self)->Eigen::VectorXd{
            auto id_vecs = self.get_ordered_telescope_ids();
            auto pix_y = self.tels.at(id_vecs[0]).camera_description.camera_geometry.pix_y;
            double focal_length = self.tels.at(id_vecs[0]).optics_description.equivalent_focal_length;
            return pix_y.array()/focal_length * 180/M_PI;
        })
        .def_prop_ro("get_pix_size", [](const SubarrayDescription& self)->double{
            auto id_vecs = self.get_ordered_telescope_ids();
            double focal_length = self.tels.at(id_vecs[0]).optics_description.effective_focal_length;
            auto pix_area = self.tels.at(id_vecs[0]).camera_description.camera_geometry.pix_area;
            return sqrt(pix_area[0])/focal_length * 180/M_PI;
        })
        .def_prop_ro("get_pix_size_equivalent", [](const SubarrayDescription& self)->double{
            auto id_vecs = self.get_ordered_telescope_ids();
            double focal_length = self.tels.at(id_vecs[0]).optics_description.equivalent_focal_length;
            auto pix_area = self.tels.at(id_vecs[0]).camera_description.camera_geometry.pix_area;
            return sqrt(pix_area[0])/focal_length * 180/M_PI;
        });

        
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
        .def_ro("neigh_matrix", &CameraGeometry::neigh_matrix)
        .def("__repr__", &CameraGeometry::print);
}

NB_MODULE(_pylast_subarray, m)
{
    bind_subarray_description(m);
}
