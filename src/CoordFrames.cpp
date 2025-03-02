#include "CoordFrames.hh"
#include "Eigen/Geometry"
#include "Eigen/Dense"
/**
 * @brief Convert the pointing direction to the telescope frame
 *        First rotate the pointing direction by the azimuth and then by the altitude
 * 
 * @param point 
 * @param target 
 * @return RepresentationType<TelescopeFrame> 
 */
auto AltAzFrame::transform_to_imp(const representation_type& point, const TelescopeFrame& target) const -> RepresentationType<TelescopeFrame>
{
    auto transformed_direction = CartesianRepresentation(target.rotation_matrix * point.transform_to_cartesian().direction);
    double x_offset = transformed_direction.direction.x() / transformed_direction.direction.z();
    double y_offset = transformed_direction.direction.y() / transformed_direction.direction.z();
    return TelescopeFrame::representation_type({x_offset, y_offset});
}


auto TelescopeFrame::transform_to_imp(const representation_type& point, const AltAzFrame& target) const -> RepresentationType<AltAzFrame>
{
    double r = point.point.norm();
    double altitude = atan(r);
    double azimuth = atan2(point.point.y(), point.point.x());
    auto transformed_direction = CartesianRepresentation(this->rotation_matrix.transpose() * SphericalRepresentation(azimuth, altitude).transform_to_cartesian().direction);
    return transformed_direction.transform_to_spherical();
}