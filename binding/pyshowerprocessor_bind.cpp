#include "GeometryReconstructor.hh"
#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/pair.h"
#include "nanobind/make_iterator.h"
#include "nanobind/stl/optional.h"
#include "ShowerProcessor.hh"
#include "nanobind/stl/unique_ptr.h"
namespace nb = nanobind;

class PublicList: public GeometryReconstructor
{
    public:
        using GeometryReconstructor::geometry;
        using GeometryReconstructor::hillas_dicts;
        using GeometryReconstructor::telescopes;
        using GeometryReconstructor::array_pointing_direction;
        using GeometryReconstructor::compute_angle_separation;
        using GeometryReconstructor::convert_to_sky;
};
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
    nb::class_<ImageQuery>(m, "ImageQuery")
        .def(nb::init<const std::string&>(), nb::arg("config"))
        .def("__call__", [](ImageQuery& self, const ImageParameters& image_parameters) {
            return self(image_parameters);
        });
    nb::class_<GeometryReconstructor>(m, "GeometryReconstructor")
        .def(nb::init<const SubarrayDescription&>(), nb::arg("subarray"))
        .def(nb::init<const SubarrayDescription&, const std::string&>(), nb::arg("subarray"), nb::arg("config_str"))
        .def("__call__", [](GeometryReconstructor& self, ArrayEvent& event) {
            self(event);
        })
        .def_rw("geometry", &PublicList::geometry)
        .def_ro("hillas_dicts", &PublicList::hillas_dicts)
        .def_ro("telescopes", &PublicList::telescopes)
        .def_ro("array_pointing_direction", &PublicList::array_pointing_direction)
        .def_static("compute_angle_separation", &PublicList::compute_angle_separation)
        .def("convert_to_sky", &PublicList::convert_to_sky);
}

NB_MODULE(_pylast_showerprocessor, m) {
    bind_showerprocessor(m);
}


