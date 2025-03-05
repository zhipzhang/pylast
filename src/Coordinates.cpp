#include "Coordinates.hh"

SphericalRepresentation CartesianRepresentation::transform_to_spherical() const
{
    double azimuth = std::atan2(-direction.y(), direction.x());
    double altitude = std::asin((direction.z())/direction.norm());
    return SphericalRepresentation(azimuth, altitude);
}

double Point2D::distance(const Line2D& line) const
{
    return line.distance(*this);
}