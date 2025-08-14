#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ImageQuery.hh"


TEST_CASE("TEST_SIMPLE_STRING_QUERY")
{
    std::string query = "hillas_intensity > 100 && hillas_length > 0";
    auto query_parser = std::make_unique<ImageQuery>(query);
    ImageParameters image_parameters;
    image_parameters.hillas.intensity = 99;
    image_parameters.hillas.length = -1;
    CHECK((*query_parser)(image_parameters) == false);
    image_parameters.hillas.length = 1;
    CHECK((*query_parser)(image_parameters) == false);
    image_parameters.hillas.intensity = 101;
    CHECK((*query_parser)(image_parameters) == true);
}
TEST_CASE("TEST_CONFIG_QUERY")
{
    std::string query = R"(
        {
            "ImageQuery": {
                "size > 100pe.": "hillas_intensity > 100",
                "positive length": "hillas_length > 0"
            }
        }
    )";
    auto query_parser = std::make_unique<ImageQuery>(query);
    ImageParameters image_parameters;
    image_parameters.hillas.intensity = 99;
    image_parameters.hillas.length = -1;
    CHECK((*query_parser)(image_parameters) == false);
    image_parameters.hillas.intensity = 101;
    image_parameters.hillas.length = 1;
    CHECK((*query_parser)(image_parameters) == true);
}