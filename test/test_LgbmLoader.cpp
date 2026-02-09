#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "LGBMModelLoader.hh"

TEST_CASE("Test LGBMModelLoader") {
    auto test_directory = std::filesystem::path(__FILE__);
    auto config_file_path = test_directory.parent_path() / "test_data" / "test_classifier_model.json";
    auto lgbm_model_loader = LGBMModelLoader(config_file_path);
    CHECK(lgbm_model_loader.IsClassification());
    CHECK(lgbm_model_loader.GetModelName() == "example_classifier");
    CHECK(lgbm_model_loader.GetFeatureNumber() == 3);
}