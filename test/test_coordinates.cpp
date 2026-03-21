#include "CoordFrames.hh"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "Coordinates.hh"

TEST_CASE("TEST_SPHERICAL_CONVERT_TO_CARTESIAN")
{
    SphericalRepresentation spherical(0, 30 * M_PI / 180);
    CartesianRepresentation cartesian = spherical.transform_to_cartesian();
    CHECK(cartesian.direction.x() == doctest::Approx(std::sqrt(3)/2));
    CHECK(cartesian.direction.y() == doctest::Approx(0));
    CHECK(cartesian.direction.z() == doctest::Approx(0.5));
}

TEST_CASE("TEST_SPHERICAL_ANGLE_SEPARATION")
{
    SphericalRepresentation spherical1(0, 30 * M_PI / 180);
    SphericalRepresentation spherical2(0, 45 * M_PI / 180);
    SphericalRepresentation spherical3(180 * M_PI / 180, 60 * M_PI / 180);
    CHECK(spherical1.angle_separation(spherical2) == doctest::Approx(15 * M_PI / 180));
    CHECK(spherical1.angle_separation(spherical3) == doctest::Approx(90 * M_PI / 180));
}
TEST_CASE("TEST_LINE_POINT_DISTANCE")
{
    Line2D line({1, 1}, {std::sqrt(2)/2, std::sqrt(2)/2});
    Point2D point({1, 0});
    CHECK(line.distance(point) == doctest::Approx(std::sqrt(2)/2));
    SUBCASE("TEST_NONNORMALIZED_DIRECTION")
    {
        Line2D line({1, 1}, {1, 1});
        Point2D point({1, 0});
        CHECK(line.distance(point) == doctest::Approx(std::sqrt(2)/2));
        CHECK(point.distance(line) == doctest::Approx(std::sqrt(2)/2));
    }
}
TEST_CASE("TEST_LINE_INTERSECTION")
{
   SUBCASE("TEST_PARALLEL_LINES")
   {
    Line2D line1({1, 1}, {1, 1});
    Line2D line2({1, 1}, {1, 1});
    CHECK(line1.intersection(line2) == std::nullopt);
   }
   SUBCASE("TEST_NONPARALLEL_LINES")
   {
    Line2D line1({1, 1}, {1, 1});
    Line2D line2({0, 1}, {1, 0});
    CHECK((line1.intersection(line2).value() == Point2D({1, 1})));
   }
   
}
TEST_CASE("TEST_TELESCOPE_FRAME")
{
    auto telescope_frame = TelescopeFrame(0, 30 * M_PI / 180);
    CHECK(telescope_frame.pointing_direction.azimuth == 0);
    CHECK(telescope_frame.pointing_direction.altitude == 30 * M_PI / 180);
    CHECK(telescope_frame.rotation_matrix(0,0) == doctest::Approx(0.5));
    CHECK(telescope_frame.rotation_matrix(0,2) == doctest::Approx(-std::sqrt(3)/2));
    CHECK(telescope_frame.rotation_matrix(1,1) == doctest::Approx(1));
    CHECK(telescope_frame.rotation_matrix(2,0) == doctest::Approx(std::sqrt(3)/2));
    CHECK(telescope_frame.rotation_matrix(2,2) == doctest::Approx(0.5));
}
TEST_CASE("TEST_TELESCOPE_FRAME_TRANS")
{
    SUBCASE("ON_AXIS_TRANSFORMATION")
    {
        auto point_direction = SkyDirection(AltAzFrame(), 0, 30 * M_PI / 180);
        auto telescope_frame = TelescopeFrame(0, 30 * M_PI / 180);
        auto camera_point = point_direction.transform_to<TelescopeFrame>(telescope_frame);
        CHECK(camera_point->x() == doctest::Approx(0));
        CHECK(camera_point->y() == doctest::Approx(0));
    }
    SUBCASE("OFF_AXIS_TRANSFORMATION")
    {
        auto point_direction = SkyDirection(AltAzFrame(), 0, (30 + 10)* M_PI / 180);
        auto telescope_frame = TelescopeFrame(0, 30 * M_PI / 180);
        auto camera_point = point_direction.transform_to(telescope_frame);
        CHECK(camera_point->x() == doctest::Approx(std::tan(10 * M_PI / 180)));
        CHECK(camera_point->y() == doctest::Approx(0));
    }
}
TEST_CASE("TEST_TELESCOPE_FRAME_TRANS_TO_SKY")
{
    auto telescope_frame = TelescopeFrame(0, 30 * M_PI / 180);
    auto sky_direction = SkyDirection(telescope_frame, std::tan(10 * M_PI / 180), 0);
    auto alt_az_direction = sky_direction.transform_to(AltAzFrame());
    CHECK(alt_az_direction->azimuth == doctest::Approx(0 * M_PI / 180));
    CHECK(alt_az_direction->altitude == doctest::Approx(40 * M_PI / 180));
}

TEST_CASE("TEST_TELESCOPE_FRAME_DISTANCE_AND_SEPARATION")
{
    auto telescope_frame = TelescopeFrame(0, 30 * M_PI / 180);
    auto skydirection1 = SkyDirection(AltAzFrame(), 5 * M_PI / 180, 35 * M_PI / 180);
    auto skydirection2 = SkyDirection(AltAzFrame(), 10 * M_PI / 180, 40 * M_PI / 180);
    auto distance = skydirection1->angle_separation(skydirection2.position);
    auto camera_point1 = skydirection1.transform_to(telescope_frame);
    auto camera_point2 = skydirection2.transform_to(telescope_frame);
    auto camera_distance = std::sqrt(std::pow(camera_point1->x() - camera_point2->x(), 2) + std::pow(camera_point1->y() - camera_point2->y(), 2));
    CHECK(distance - camera_distance < 1e-2); // They are quite close
}
TEST_CASE("TEST_TWO_TRANSFORMATION")
{
    auto telescope_frame = TelescopeFrame(0, 30 * M_PI / 180);
    auto skydirection = SkyDirection(AltAzFrame(), 5 * M_PI / 180, 35 * M_PI / 180);
    auto camera_point = skydirection.transform_to(telescope_frame);
    auto alt_az_direction = camera_point.transform_to(AltAzFrame());
    CHECK(alt_az_direction->azimuth == doctest::Approx(5 * M_PI / 180));
    CHECK(alt_az_direction->altitude == doctest::Approx(35 * M_PI / 180));
    auto camera_point2 = alt_az_direction.transform_to(telescope_frame);
    CHECK(camera_point2->x() == doctest::Approx(camera_point->x()));
    CHECK(camera_point2->y() == doctest::Approx(camera_point->y()));
}