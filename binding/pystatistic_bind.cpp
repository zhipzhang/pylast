#include "Statistics.hh"
#include "nanobind/nanobind.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/variant.h"
#include "nanobind/stl/shared_ptr.h"
#include "nanobind/eigen/dense.h"
#include "nanobind/stl/vector.h"
namespace nb = nanobind;

using namespace eigen_histogram;
NB_MODULE(_pystatistic, m){
    nb::class_<Statistics>(m, "Statistics")
        .def(nb::init<>())
        .def_ro("histograms", &Statistics::histograms)
        .def("add_histogram", [](Statistics& self, const std::string& name, nb::object hist_obj) {
            if (nb::isinstance<Histogram1D<float>>(hist_obj)) {
                auto hist = nb::cast<Histogram1D<float>>(hist_obj);
                self.add_histogram(name, hist);
            } else if (nb::isinstance<Histogram2D<float>>(hist_obj)) {
                auto hist = nb::cast<Histogram2D<float>>(hist_obj);
                self.add_histogram(name, hist);
            } else {
                throw std::runtime_error("Unsupported histogram type");
            }
        });
    nb::class_<Histogram<float>>(m, "Histogram")
        .def("reset", &Histogram<float>::reset);
    nb::class_<Histogram1D<float>, Histogram<float>>(m, "Histogram1D")
        .def_prop_ro("bins", &Histogram1D<float>::bins)
        .def_prop_ro("underflow", &Histogram1D<float>::underflow)
        .def_prop_ro("overflow", &Histogram1D<float>::overflow)
        .def_prop_ro("centers", [](Histogram1D<float>& self)->Eigen::VectorXf{
            return self.centers();
        })
        .def_prop_ro("vec_centers", &Histogram1D<float>::vec_centers)
        .def_prop_ro("values", &Histogram1D<float>::values)
        .def("get_bin_center", &Histogram1D<float>::get_bin_center)
        .def("get_bin_content", &Histogram1D<float>::get_bin_content)
        .def("__getitem__", &Histogram1D<float>::operator())
        .def("fill", [](Histogram1D<float>& self, float value) {
            self.fill(value);
        })
        .def("fill", nb::overload_cast<float, float>(&Histogram1D<float>::fill), "fill a single value with a weight")
        .def("fill", [](Histogram1D<float>& self, const Eigen::Ref<const Eigen::VectorXf>& values, const Eigen::Ref<const Eigen::VectorXf>& weights) {
            self.fill(values, weights);
        })
        .def("fill", [](Histogram1D<float>& self, const Eigen::Ref<const Eigen::VectorXf>& values) {
            Eigen::VectorXf weights = Eigen::VectorXf::Ones(values.size());
            self.fill(values, weights);
        })
        .def("center", &Histogram1D<float>::center);
    nb::class_<Histogram2D<float>, Histogram<float>>(m, "Histogram2D")
        .def_prop_ro("x_bins", &Histogram2D<float>::x_bins)
        .def_prop_ro("y_bins", &Histogram2D<float>::y_bins)
        .def_prop_ro("underflow_x", &Histogram2D<float>::underflow_x)
        .def_prop_ro("overflow_x", &Histogram2D<float>::overflow_x)
        .def_prop_ro("underflow_y", &Histogram2D<float>::underflow_y)
        .def_prop_ro("overflow_y", &Histogram2D<float>::overflow_y)
        .def_prop_ro("underflow_xy", &Histogram2D<float>::underflow_xy)
        .def_prop_ro("overflow_xy", &Histogram2D<float>::overflow_xy)
        .def("__call__", &Histogram2D<float>::operator())
        .def("fill", [](Histogram2D<float>& self, float x, float y) {
            self.fill(x, y);
        })
        .def("fill", [](Histogram2D<float>& self, float x, float y, float weight) {
            self.fill(x, y, weight);
        });
    m.def("make_regular_histogram", &make_regular_histogram<float>, nb::arg("min"), nb::arg("max"), nb::arg("bins"));
    m.def("make_regular_histogram2d", &make_regular_histogram_2d<float>, nb::arg("min_x"), nb::arg("max_x"), nb::arg("bins_x"), nb::arg("min_y"), nb::arg("max_y"), nb::arg("bins_y"));
}

