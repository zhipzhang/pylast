#include "CoordFrames.hh"
#include "Coordinates.hh"
#include "GeometryReconstructor.hh"
#include "ImageParameters.hh"
#include <unordered_map>
#include <stdexcept>



void GeometryReconstructor::operator()(ArrayEvent& event)
{
    hillas_dicts.clear();
    rounded_hillas_dicts.clear();
    rounded_telescopes.clear();
    Reconstructor::operator()(event);
    nominal_frame = std::make_unique<TelescopeFrame>(array_pointing_direction.azimuth, array_pointing_direction.altitude);
    for(auto tel_id: telescopes)
    {
        if(!event.pointing->tels.contains(tel_id))
        {
            throw std::runtime_error("telescope " + std::to_string(tel_id) + " not found in pointing");
        }
        telescope_pointing[tel_id] = SphericalRepresentation(event.pointing->tels[tel_id]->azimuth, event.pointing->tels[tel_id]->altitude);
        if(use_fake_hillas)
        {
            hillas_dicts[tel_id] = event.simulation->tels[tel_id]->image_parameters.hillas;
        }
        else
        {
            hillas_dicts[tel_id] = event.dl1->tels[tel_id]->image_parameters.hillas;
        }
    }
    if(event.rounded_tel_hillas.size() > 0)
    {
        for(auto& [tel_id, rounded_hillas]: event.rounded_tel_hillas)
        {
            if(hillas_dicts.contains(tel_id))
            {
                rounded_hillas_dicts[tel_id] = rounded_hillas;
                rounded_telescopes.push_back(tel_id);
            }
        }
    }
    return;
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
std::unordered_map<int, Point2D> GeometryReconstructor::get_tiled_tel_position(const TiltedGroundFrame& tilted_frame)
{
    std::unordered_map<int, Point2D> tiled_tel_positions;
    for(const auto tel_id: telescopes)
    {
        auto tel_pos = CartesianPoint(subarray.tel_positions.at(tel_id));
        auto tilted_tel_pos = tel_pos.transform_to_tilted(tilted_frame);
        tiled_tel_positions.emplace(tel_id, Point2D(tilted_tel_pos.x(), tilted_tel_pos.y()));
    }
    return tiled_tel_positions;
}

std::pair<double, double> GeometryReconstructor::project_to_ground(const Eigen::Vector3d& intersection_position, const SkyDirection<AltAzFrame>& direction)
{
    auto direction_vector = direction->transform_to_cartesian();
    // Calculate the intersection point with the ground (z=0)
    // If the direction is parallel to the ground, return the current position
    if (std::abs(direction_vector.direction.z()) < 1e-10) {
        return {intersection_position.x(), intersection_position.y()};
    }
    
    // Calculate how far we need to go to reach z=0
    double t = -intersection_position.z() / direction_vector.direction.z();
    
    // Calculate the ground intersection point
    double ground_x = intersection_position.x() + t * direction_vector.direction.x();
    double ground_y = intersection_position.y() + t * direction_vector.direction.y();
    
    return {ground_x, ground_y};
}