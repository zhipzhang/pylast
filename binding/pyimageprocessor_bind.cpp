#include "ImageProcessor.hh"
#include "nanobind/eigen/dense.h"
#include "nanobind/make_iterator.h"
#include "nanobind/nanobind.h"
#include "nanobind/stl/optional.h"
#include "nanobind/stl/pair.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/vector.h"

namespace nb = nanobind;

void bind_imageprocessor(nb::module_ &m) {
  nb::class_<ImageProcessor>(m, "ImageProcessor")
      .def(nb::init<const SubarrayDescription &, const std::string &>(),
           nb::arg("subarray"), nb::arg("config_str"))
      .def(nb::init<const SubarrayDescription &>(), nb::arg("subarray"))
      .def("__call__",
           [](ImageProcessor &self, ArrayEvent &event) { self(event); })
      .def("image_clean",
           [](ImageProcessor &self, const CameraGeometry &camera_geometry,
              const Eigen::VectorXd &image) {
             return self.image_clean(camera_geometry, image);
           })
      .def_static("dilate_image",
                  [](const CameraGeometry &camera_geometry,
                     Eigen::Vector<bool, -1> &image_mask) {
                    ImageProcessor::dilate_image(camera_geometry, image_mask);
                  })
      .def_static("hillas_parameter",
                  [](CameraGeometry &camera_geometry,
                     const Eigen::VectorXd &masked_image) {
                    return ImageProcessor::hillas_parameter(camera_geometry,
                                                            masked_image);
                  })
      .def_static("leakage_parameter",
                  [](CameraGeometry &camera_geometry,
                     const Eigen::VectorXd &masked_image) {
                    return ImageProcessor::leakage_parameter(camera_geometry,
                                                             masked_image);
                  })
      .def_static("concentration_parameter",
                  [](const CameraGeometry &camera_geometry,
                     const Eigen::VectorXd &masked_image,
                     const HillasParameter &hillas_parameter) {
                    return ImageProcessor::concentration_parameter(
                        camera_geometry, masked_image, hillas_parameter);
                  });
}

NB_MODULE(_pylast_imageprocessor, m) { bind_imageprocessor(m); }