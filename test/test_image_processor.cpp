#include <limits>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ImageCleaner.hh"
#include "ImageProcessor.hh"
#include "ConfigSystem.hh"
#include "ConfigMacros.hh"

TEST_CASE("TEST_CONFIGURABLE")
{
    TailcutsCleaner cleaner;
    CHECK(cleaner.get_picture_thresh() == 10);
    CHECK(cleaner.get_boundary_thresh() == 5);
    CHECK(cleaner.get_keep_isolated_pixels() == false);
    CHECK(cleaner.get_min_number_picture_neighbors() == 2);
    SUBCASE("TEST_USER_CONFIG_STRING")
    {
        std::string config_str = R"(
        {
            "picture_thresh": 30,
            "boundary_thresh": 10,
            "keep_isolated_pixels": true,
            "min_number_picture_neighbors": 3
        }
        )";
        TailcutsCleaner cleaner2(config_str);
        CHECK(cleaner2.get_picture_thresh() == 30);
        CHECK(cleaner2.get_boundary_thresh() == 10);
        CHECK(cleaner2.get_keep_isolated_pixels() == true);
        CHECK(cleaner2.get_min_number_picture_neighbors() == 3);
    }
    SUBCASE("TEST_USER_CONFIG_JSON")
    {
        json config = TailcutsCleaner::get_default_config();
        config["picture_thresh"] = 30;
        config["boundary_thresh"] = 10;
        config["keep_isolated_pixels"] = true;
        config["min_number_picture_neighbors"] = 3;
        TailcutsCleaner cleaner2(config);
        CHECK(cleaner2.get_picture_thresh() == 30);
        CHECK(cleaner2.get_boundary_thresh() == 10);
        CHECK(cleaner2.get_keep_isolated_pixels() == true);
        CHECK(cleaner2.get_min_number_picture_neighbors() == 3);
    }
}
TEST_CASE("TEST_TAILCUTS_CLEAN")
{
    int num_pixels = 16;
    std::vector<double> pix_x;
    std::vector<double> pix_y;
    std::vector<double> pix_area;
    std::vector<int> pix_type;
    for(int j = 0; j < 4; j++)
    {
        for(int i = 0; i < 4; i++)
        {
            pix_x.push_back(i);
            pix_y.push_back(j);
            pix_area.push_back(1);
            pix_type.push_back(2);
        }
    }
    CameraGeometry camera("test", num_pixels, pix_x.data(), pix_y.data(), pix_area.data(), pix_type.data(), 0);
    // Test center pixel (5) should have 4 neighbors
    SUBCASE("EMPTY_IMAGE")
    {
        auto empty_image = Eigen::VectorXd::Zero(num_pixels);
        auto clean_image = TailcutsCleaner::tailcuts_clean(camera, empty_image, 1, 1);
        // Check that all pixels are marked as not clean (0) when image is empty
        CHECK(clean_image.array().sum() == 0);
    }
    SUBCASE("CONSTANT_IMAGE")
    {
        auto constant_image = Eigen::VectorXd::Constant(num_pixels, 10);
        auto clean_image = TailcutsCleaner::tailcuts_clean(camera, constant_image, 1, 1);
        CHECK(clean_image.array().cast<int>().sum() == num_pixels);
    }
    SUBCASE("SOLO_ABOVE_THRESHOLD")
    {
        Eigen::VectorXd solo_image = Eigen::VectorXd::Constant(num_pixels, 5);
        solo_image(10) = 10;
        auto clean_image = TailcutsCleaner::tailcuts_clean(camera, solo_image, 8, 1);
        CHECK(clean_image.array().cast<int>().sum() == 5);
        CHECK(clean_image(6) == 1);
        CHECK(clean_image(9) == 1);
        CHECK(clean_image(10) == 1);
        CHECK(clean_image(11) == 1);
        CHECK(clean_image(14) == 1);
    }
    SUBCASE("KEEP_ISOLATED_PIXELS")
    {
        Eigen::VectorXd keep_isolated_image = Eigen::VectorXd::Constant(num_pixels, 1);
        keep_isolated_image(10) = 10;
        keep_isolated_image(6) = 5;
        keep_isolated_image(9) = 5;
        keep_isolated_image(0) = 10;
        auto clean_image = TailcutsCleaner::tailcuts_clean(camera, keep_isolated_image, 8, 2, true);
        CHECK(clean_image.array().cast<int>().sum() == 4);
        CHECK(clean_image(6) == 1);
        CHECK(clean_image(9) == 1);
        CHECK(clean_image(10) == 1);
        CHECK(clean_image(0) == 1);
    }
    // At least have min_number_picture_neighbors neighbors above the threshold
    SUBCASE("NO_KEEP_ISOLATED_PIXELS")
    {
        Eigen::VectorXd no_keep_isolated_image = Eigen::VectorXd::Constant(num_pixels, 1);
        no_keep_isolated_image(10) = 10;
        no_keep_isolated_image(6) = 10;
        no_keep_isolated_image(9) = 10;
        no_keep_isolated_image(0) = 10;
        auto clean_image = TailcutsCleaner::tailcuts_clean(camera, no_keep_isolated_image, 8, 2, false, 2);
        CHECK(clean_image.array().cast<int>().sum() == 3);
        CHECK(clean_image(0) == 0);
        CHECK(clean_image(6) == 1);
        CHECK(clean_image(9) == 1);
        CHECK(clean_image(10) == 1);
    }
}
TEST_CASE("TEST_LEAKAGE_PARAMETERS")
{
    int num_pixels = 25;
    std::vector<double> pix_x;
    std::vector<double> pix_y;
    std::vector<double> pix_area;
    std::vector<int> pix_type;
    for(int j = 0; j < 5; j++)  
    {
        for(int i = 0; i < 5; i++)
        {
            pix_x.push_back(i);
            pix_y.push_back(j);
            pix_area.push_back(1);
            pix_type.push_back(2);
        }
    }
    CameraGeometry camera("test", num_pixels, pix_x.data(), pix_y.data(), pix_area.data(), pix_type.data(), 0);
    SUBCASE("EMPTY_IMAGE")
    {
        auto empty_image = Eigen::VectorXd::Zero(num_pixels);
        auto leakage_parameters = ImageProcessor::leakage_parameter(camera, empty_image);
        CHECK(std::isnan(leakage_parameters.pixels_width_1));
        CHECK(std::isnan(leakage_parameters.pixels_width_2));
        CHECK(std::isnan(leakage_parameters.intensity_width_1));
        CHECK(std::isnan(leakage_parameters.intensity_width_2));
    }
    SUBCASE("CONSTANT_IMAGE")
    {
        auto constant_image = Eigen::VectorXd::Constant(num_pixels, 10);
        auto leakage_parameters = ImageProcessor::leakage_parameter(camera, constant_image);
        CHECK(leakage_parameters.pixels_width_1 == 16/25.0);
        CHECK(leakage_parameters.pixels_width_2 == 24/25.0);
        CHECK(leakage_parameters.intensity_width_1 == 160/250.0);
        CHECK(leakage_parameters.intensity_width_2 == 240/250.0);
    }
    SUBCASE("NORMAL_IMAGE")
    {
        Eigen::VectorXd normal_image = Eigen::VectorXd::Constant(num_pixels, 1);
        normal_image(0) = 10;
        auto leakage_parameters = ImageProcessor::leakage_parameter(camera, normal_image);
        CHECK(leakage_parameters.pixels_width_1 == 16/25.0);
        CHECK(leakage_parameters.pixels_width_2 == 24/25.0);
        CHECK(leakage_parameters.intensity_width_1 == 1.0*(15 + 10)/(24 + 10));
        CHECK(leakage_parameters.intensity_width_2 == 1.0*(23 + 10)/(24 + 10));
    }
}
TEST_CASE("TEST_MORPHOLOGY_PARAMETERS")
{
    int num_pixels = 25;
    std::vector<double> pix_x;
    std::vector<double> pix_y;
    std::vector<double> pix_area;
    std::vector<int> pix_type;
    for(int j = 0; j < 5; j++)  
    {
        for(int i = 0; i < 5; i++)
        {
            pix_x.push_back(i);
            pix_y.push_back(j);
            pix_area.push_back(1);
            pix_type.push_back(2);
        }
    }
    CameraGeometry camera("test", num_pixels, pix_x.data(), pix_y.data(), pix_area.data(), pix_type.data(), 0);
    SUBCASE("EMPTY_MASk")
    {
        auto empty_mask = Eigen::Vector<bool, -1>::Zero(num_pixels);
        auto morphology_parameters = ImageProcessor::morphology_parameter(camera, empty_mask);
        CHECK(morphology_parameters.n_pixels == 0);
        CHECK(morphology_parameters.n_small_islands == 0);
        CHECK(morphology_parameters.n_medium_islands == 0);
        CHECK(morphology_parameters.n_large_islands == 0);
    }
    SUBCASE("FULL_MASK")
    {
        auto full_mask = Eigen::Vector<bool, -1>::Constant(num_pixels, true);
        auto morphology_parameters = ImageProcessor::morphology_parameter(camera, full_mask);
        CHECK(morphology_parameters.n_pixels == num_pixels);
        CHECK(morphology_parameters.n_islands == 1);
        CHECK(morphology_parameters.n_medium_islands == 1);
    }
    SUBCASE("NORMAL_MASK")
    {
        Eigen::Vector<bool, -1> normal_mask = Eigen::Vector<bool, -1>::Zero(num_pixels);
        normal_mask(0) = true;
        normal_mask(1) = true;
        normal_mask(2) = true;
        normal_mask(3) = true;
        normal_mask(4) = true;
        normal_mask(20) = true;
        normal_mask(21) = true;
        normal_mask(22) = true;
        normal_mask(23) = true;
        normal_mask(24) = true;
        auto morphology_parameters = ImageProcessor::morphology_parameter(camera, normal_mask);
        CHECK(morphology_parameters.n_pixels == 10);
        CHECK(morphology_parameters.n_islands == 2);
        CHECK(morphology_parameters.n_small_islands == 2);
        CHECK(morphology_parameters.n_medium_islands == 0);
        CHECK(morphology_parameters.n_large_islands == 0);
    }
}
TEST_CASE("TEST_HILLAS_PARAMETER")
{
    int num_pixels = 16;
    std::vector<double> pix_x;
    std::vector<double> pix_y;
    std::vector<double> pix_area;
    std::vector<int> pix_type;
    for(int j = 0; j < 4; j++)
    {
        for(int i = 0; i < 4; i++)
        {
            pix_x.push_back(i); 
            pix_y.push_back(j);
            pix_area.push_back(1);
            pix_type.push_back(2);
        }
    }
    CameraGeometry camera("test", num_pixels, pix_x.data(), pix_y.data(), pix_area.data(), pix_type.data(), 0);
    SUBCASE("ONLY_DIAGONAL_PIXELS")
    {
        Eigen::VectorXd diagonal_image = Eigen::VectorXd::Zero(num_pixels);
        diagonal_image(0) = 1;
        diagonal_image(5) = 1;
        diagonal_image(10) = 1;
        diagonal_image(15) = 1;
        auto hillas_parameters = ImageProcessor::hillas_parameter(camera, diagonal_image);
        CHECK(hillas_parameters.psi == doctest::Approx(M_PI/4));
        CHECK(hillas_parameters.intensity == 4);
        CHECK(hillas_parameters.x == doctest::Approx(1.5));
        CHECK(hillas_parameters.y == doctest::Approx(1.5));
    }
}

TEST_CASE("TEST_DILATE_IMAGE")
{
    int num_pixels = 16;
    std::vector<double> pix_x;
    std::vector<double> pix_y;
    std::vector<double> pix_area;
    std::vector<int> pix_type;
    for(int j = 0; j < 4; j++)
    {
        for(int i = 0; i < 4; i++)
        {
            pix_x.push_back(i);
            pix_y.push_back(j);
            pix_area.push_back(1);
            pix_type.push_back(2);
        }
    }
    CameraGeometry camera("test", num_pixels, pix_x.data(), pix_y.data(), pix_area.data(), pix_type.data(), 0); 
    Eigen::Vector<bool, -1> image_mask = Eigen::Vector<bool, -1>::Zero(num_pixels);
    image_mask(0) = true;
    ImageProcessor::dilate_image(camera, image_mask);
    CHECK(image_mask.array().cast<int>().sum() == 3);
    CHECK(image_mask(1) == true);
    CHECK(image_mask(4) == true);
    CHECK(image_mask(0) == true);
}

TEST_CASE("TEST_CUT_RADIUS")
{
    int num_pixels = 16;
    std::vector<double> pix_x;
    std::vector<double> pix_y;
    std::vector<double> pix_area;
    std::vector<int> pix_type;
    for(int j = 0; j < 4; j++)
    {
        for(int i = 0; i < 4; i++)
        {
            pix_x.push_back(i);
            pix_y.push_back(j);
            pix_area.push_back(1);
            pix_type.push_back(2);
        }
    }
    CameraGeometry camera("test", num_pixels, pix_x.data(), pix_y.data(), pix_area.data(), pix_type.data(), 0); 
    auto pixel_mask = ImageProcessor::cut_pixel_distance(camera, 1, 2 * 180 / M_PI);
    CHECK(pixel_mask.array().cast<int>().sum() == 6);
    auto pixel_mask2 = ImageProcessor::cut_pixel_distance(camera, 1, 1 * 180 / M_PI);
    CHECK(pixel_mask2.array().cast<int>().sum() ==3);
}