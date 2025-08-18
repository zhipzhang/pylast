#include "CoordFrames.hh"
#include "GeometryReconstructor.hh"
struct ShowerDiection
{
    Point2D core_position;
    SkyDirection<AltAzFrame> direction;
};

/**
    This method is the Algorithm-6 in Hofmann's Paper!
*/
class HillasSumReconstructor: public GeometryReconstructor
{
    public:
        virtual std::string name() const override { return "HillasSumReconstructor"; }

        std::pair<Point2D, double> project_plane_to_camera1(ShowerDiection shower_info, Point2D tiled_tel_pos);
        std::pair<Point2D, double> project_plane_to_camera2(ShowerDiection shower_info, Point2D tiled_tel_pos);
};