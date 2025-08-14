#include "CoordFrames.hh"
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
#include "MLReconstructor.hh"
#include "Coordinates.hh"
#include "nanobind/eigen/dense.h"
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
        using GeometryReconstructor::convert_to_fov;
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
        .def("convert_to_sky", &PublicList::convert_to_sky)
        .def("convert_to_fov", &PublicList::convert_to_fov);
    nb::class_<MLReconstructor>(m, "MLReconstructor")
        .def(nb::init<const std::string&>(), nb::arg("config_str"))
        .def_ro("telescopes", &MLReconstructor::telescopes)
        .def_ro("tel_rec_params", &MLReconstructor::tel_rec_params)
        .def_ro("array_pointing_direction", &MLReconstructor::array_pointing_direction)
        .def("__call__", [](MLReconstructor& self, ArrayEvent& event) {
            self(event);
        });
}

void bind_coordinates(nb::module_ &m) {
    nb::class_<Point2D>(m, "Point2D")
        .def(nb::init<double, double>(), nb::arg("x"), nb::arg("y"))
        .def_prop_ro("x", &Point2D::x)
        .def_prop_ro("y", &Point2D::y);
    nb::class_<CartesianRepresentation>(m, "CartesianRepresentation")
        .def(nb::init<double, double, double>(), nb::arg("x"), nb::arg("y"), nb::arg("z"));
    nb::class_<SphericalRepresentation>(m, "SphericalRepresentation")
        .def(nb::init<double, double>(), nb::arg("azimuth"), nb::arg("altitude"))
        .def_ro("azimuth", &SphericalRepresentation::azimuth)
        .def_ro("altitude", &SphericalRepresentation::altitude);
    nb::class_<TelescopeFrame>(m, "TelescopeFrame")
        .def(nb::init<double, double>(), nb::arg("azimuth"), nb::arg("altitude"))
        .def("transform_to", [](TelescopeFrame& self, const Point2D& point, const AltAzFrame& target) {
            return self.transform_to(point, target);
        });
    nb::class_<CartesianPoint>(m, "CartesianPoint")
        .def(nb::init<double, double, double>(), nb::arg("x"), nb::arg("y"), nb::arg("z"))
        .def("transform_to_tilted", [](CartesianPoint& self, const TiltedGroundFrame& target) {
            return self.transform_to_tilted(target);
        })
        .def("transform_to_ground", [](CartesianPoint& self, const TiltedGroundFrame& target) {
            return self.transform_to_ground(target);
        });
    nb::class_<TiltedGroundFrame, TelescopeFrame>(m, "TiltedGroundFrame")
        .def(nb::init<double, double>(), nb::arg("azimuth"), nb::arg("altitude"))
        .def("transform_to", [](TiltedGroundFrame& self, const Point2D& point, const AltAzFrame& target) {
            return self.transform_to(point, target);
        });
    nb::class_<AltAzFrame>(m, "AltAzFrame")
        .def(nb::init<>())
        .def("transform_to", [](const AltAzFrame& self, const SphericalRepresentation& point, const TelescopeFrame& target) {
            return self.transform_to(point, target);
        });
    nb::class_<SkyDirection<AltAzFrame>>(m, "SkyDirection")
        .def(nb::init<AltAzFrame, double, double>(), nb::arg("frame") = AltAzFrame(), nb::arg("azimuth"), nb::arg("altitude"))
        .def_prop_ro("azimuth", [](SkyDirection<AltAzFrame>& self) {
            return self.position.azimuth;
        })
        .def_prop_ro("altitude", [](SkyDirection<AltAzFrame>& self) {
            return self.position.altitude;
        })
        .def("transform_to", [](SkyDirection<AltAzFrame>& self, const TelescopeFrame& target) {
            return self.transform_to(target);
        });
    nb::class_<SkyDirection<TelescopeFrame>>(m, "TelescopeOffset")
        .def(nb::init<TelescopeFrame, double, double>(), nb::arg("frame"), nb::arg("offset_x"), nb::arg("offset_y"))
        .def_prop_ro("x_off", [](SkyDirection<TelescopeFrame>& self) {
            return self.position.x();
        })
        .def_prop_ro("y_off", [](SkyDirection<TelescopeFrame>& self) {
            return self.position.y();
        });
    
}
NB_MODULE(_pylast_showerprocessor, m) {
    bind_showerprocessor(m);
    bind_coordinates(m);
}


