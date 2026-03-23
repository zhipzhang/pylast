#include "Utils.hh"
#include "doctest/doctest.h"

TEST_CASE("Utils_Test")
{
    CHECK(Utils::point_line_distance(std::array<double,3>{0,0,0}, std::array<double,3>{0,0,0}, std::array<double,3>{1,0,0}) == 0);
    CHECK(Utils::point_line_distance(std::array<double,3>{0,0,0}, std::array<double,3>{10,0,0}, std::array<double,3>{0,1,0}) == 10);
}