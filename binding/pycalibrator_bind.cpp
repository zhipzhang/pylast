#include "Calibration.hh"
#include "ImageExtractor.hh"
#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/pair.h"
#include "SubarrayDescription.hh"
#include "nanobind/stl/unique_ptr.h"
#include "nanobind/stl/string.h"
#include "nanobind/eigen/dense.h"
#include "PrototypeCalibrator.hh"

namespace nb = nanobind;

void bind_calibrator(nb::module_ &m)
{
    nb::class_<Calibrator>(m, "Calibrator")
        .def(nb::init<const SubarrayDescription&, const std::string&>(), nb::arg("subarray"), nb::arg("config_str"))
        .def(nb::init<const SubarrayDescription&>(), nb::arg("subarray"))
        .def("__call__", [](Calibrator& self, ArrayEvent& event) {
            self(event);
        })
        .def("__repr__", [](Calibrator& self) {
            return "Calibrator:\n  Config: " + self.get_config_str();
        });
    nb::class_<PrototypeCalibrator>(m, "PrototypeCalibrator")
        .def(nb::init<>())
        .def(nb::init<const std::string&>(), nb::arg("config_str"))
        .def("registerParams", &PrototypeCalibrator::registerParams)
        .def("setUp", &PrototypeCalibrator::setUp)
        .def("__call__", [](PrototypeCalibrator& self, ArrayEvent& event) {
            self(event);
        })
        .def(
            "extract_waveform_base",
            [](PrototypeCalibrator& self, const Eigen::Ref<const Eigen::VectorXf>& waveform, int window_start, int window_end) {
                return self.extract_waveform_base(waveform, window_start, window_end);
            },
            nb::arg("waveform"),
            nb::arg("window_start"),
            nb::arg("window_size"))
        .def(
            "integrate_waveform",
            [](PrototypeCalibrator& self, const Eigen::Ref<const Eigen::VectorXf>& waveform, int window_start,
               int window_end) {
                return self.integrate_waveform(waveform, window_start, window_end);
            },
            nb::arg("waveform"),
            nb::arg("window_start"),
            nb::arg("window_end"))
        .def(
            "extract_waveform_peak",
            [](PrototypeCalibrator& self, const Eigen::Ref<const Eigen::VectorXf>& waveform) {
                return self.extract_waveform_peak(waveform);
            },
            nb::arg("waveform"))
        .def(
            "advanced_process",
            [](PrototypeCalibrator& self, ArrayEvent& event) {
                self.advanced_process(event);
            },
            nb::arg("event"))
        .def_static(
            "median_filter",
            [](const Eigen::Ref<const Eigen::VectorXf>& waveform, int kernel_size) {
                return PrototypeCalibrator::median_filter(waveform, kernel_size);
            },
            nb::arg("waveform"),
            nb::arg("kernel_size"))
        .def_static(
            "sliding_average",
            [](const Eigen::Ref<const Eigen::VectorXf>& waveform, int window_size) {
                return PrototypeCalibrator::sliding_average(Eigen::Map<const Eigen::VectorXf>(waveform.data(), waveform.size()), window_size);
            },
            nb::arg("waveform"),
            nb::arg("window_size"))
        .def_static(
            "savitzky_golay_smoothing",
            [](const Eigen::Ref<const Eigen::VectorXf>& waveform, int window_size, int order) {
                return PrototypeCalibrator::savitzky_golay_smoothing(waveform, window_size, order);
            },
            nb::arg("waveform"),
            nb::arg("window_size"),
            nb::arg("order"));
}
NB_MODULE(_pylast_calibrator, m)
{
    bind_calibrator(m);
}
