#include "Coordinates.hh"

SphericalRepresentation CartesianRepresentation::transform_to_spherical() const
{
    return SphericalRepresentation(std::atan2(direction.y(), direction.x()), std::atan(direction.norm()));
}

double Point2D::distance(const Line2D& line) const
{
    return line.distance(*this);
}