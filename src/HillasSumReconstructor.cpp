#include "HillasSumReconstructor.hh"
#include "Coordinates.hh"


std::pair<Point2D, double> HillasSumReconstructor::project_plane_to_camera1(ShowerDiection shower_info, Point2D tiled_tel_pos)
{
    // Implementation of the projection algorithm
    // This is a placeholder implementation
    auto [offset_x, offset_y] = convert_to_fov(shower_info.direction->altitude, shower_info.direction->azimuth);
    auto core_direction = CartesianRepresentation()

}

std::pair<Point2D, double> HillasSumReconstructor::project_plane_to_camera2(ShowerDiection shower_info, Point2D tiled_tel_pos)
{
    // Implementation of the projection algorithm
    // This is a placeholder implementation
    auto [offset_x, offset_y] = convert_to_fov(shower_info.direction->altitude, shower_info.direction->azimuth);

}