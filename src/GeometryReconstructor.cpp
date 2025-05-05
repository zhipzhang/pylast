#include "CoordFrames.hh"
#include "Coordinates.hh"
#include "GeometryReconstructor.hh"
#include "ImageParameters.hh"
#include <unordered_map>


json GeometryReconstructor::get_default_config()
{
    auto default_config = R"({
        "ImageQuery": {
            "100p.e.": "hillas_intensity > 100",
            "less leakage": "leakage_intensity_width_2 < 0.3"
        }
    })";
    return from_string(default_config);
}
void GeometryReconstructor::configure(const json& config)
{
    std::string image_query_config = config["ImageQuery"].dump();
    query_ = std::make_unique<ImageQuery>(image_query_config);
}
void GeometryReconstructor::operator()(ArrayEvent& event)
{
    if(!event.dl1.has_value())
    {
        throw std::runtime_error("dl1  level event not found");
    }
    if(!event.dl2.has_value())
    {
        event.dl2 = DL2Event();
    }
    hillas_dicts.clear();
    telescopes.clear();
    array_pointing_direction = SphericalRepresentation(event.pointing->array_azimuth, event.pointing->array_altitude);
    nominal_frame = std::make_unique<TelescopeFrame>(SphericalRepresentation(event.pointing->array_azimuth, event.pointing->array_altitude));
    for(const auto& [tel_id, dl1]: event.dl1->tels)
    {
        if((*query_)(dl1->image_parameters))
        {
            hillas_dicts[tel_id] = dl1->image_parameters.hillas;
            telescope_pointing[tel_id] = SphericalRepresentation(event.pointing->tels[tel_id]->azimuth, event.pointing->tels[tel_id]->altitude);
            telescopes.push_back(tel_id);
        }
    }
}
std::pair<double, double> GeometryReconstructor::convert_to_sky(double fov_x, double fov_y)
{
    auto rec_direction = SkyDirection(*nominal_frame, fov_x, fov_y).transform_to(AltAzFrame());
    return std::make_pair(rec_direction->azimuth, rec_direction->altitude);
}

std::pair<double, double> GeometryReconstructor::convert_to_fov(double alt, double az)
{
    auto camera_position = SkyDirection(AltAzFrame(), az, alt).transform_to(*nominal_frame);
    return std::make_pair(camera_position->x(), camera_position->y());
}
double GeometryReconstructor::compute_angle_separation(double az1, double alt1, double az2, double alt2)
{
    auto direction1 = SkyDirection(AltAzFrame(), az1, alt1);
    auto direction2 = SkyDirection(AltAzFrame(), az2, alt2);
    return direction1->angle_separation(direction2.position);
}