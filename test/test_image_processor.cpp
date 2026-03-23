#include <limits>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "ConfigMacros.hh"
#include "ConfigSystem.hh"
#include "ImageCleaner.hh"
#include "ImageProcessor.hh"
#include "doctest/doctest.h"

// Utility to generate a grid camera and return CameraGeometry object
CameraGeometry create_grid_camera(int w, int h) {
    int num_pixels = w * h;
    std::vector<double> pix_x, pix_y, pix_area(num_pixels, 1);
    std::vector<int> pix_type(num_pixels, 2);
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            pix_x.push_back(i);
            pix_y.push_back(j);
        }
    return CameraGeometry("test", num_pixels, pix_x.data(), pix_y.data(),
                         pix_area.data(), pix_type.data(), 0, false);
}

TEST_CASE("TEST_CONFIGURABLE") {
    TailcutsCleaner cleaner;
    CHECK(cleaner.get_picture_thresh() == 10);
    CHECK(cleaner.get_boundary_thresh() == 5);
    CHECK(cleaner.get_keep_isolated_pixels() == false);
    CHECK(cleaner.get_min_number_picture_neighbors() == 2);

    auto check_cleaner = [](const TailcutsCleaner &c, int pic, int bound, bool iso, int min_pic_n) {
        CHECK(c.get_picture_thresh() == pic);
        CHECK(c.get_boundary_thresh() == bound);
        CHECK(c.get_keep_isolated_pixels() == iso);
        CHECK(c.get_min_number_picture_neighbors() == min_pic_n);
    };

    SUBCASE("TEST_USER_CONFIG_STRING") {
        std::string config_str = R"({
                "picture_thresh": 30,
                "boundary_thresh": 10,
                "keep_isolated_pixels": true,
                "min_number_picture_neighbors": 3
            })";
        TailcutsCleaner cleaner2(config_str);
        check_cleaner(cleaner2, 30, 10, true, 3);
    }
    SUBCASE("TEST_USER_CONFIG_JSON") {
        json config = TailcutsCleaner::get_default_config();
        config["picture_thresh"] = 30;
        config["boundary_thresh"] = 10;
        config["keep_isolated_pixels"] = true;
        config["min_number_picture_neighbors"] = 3;
        TailcutsCleaner cleaner2(config);
        check_cleaner(cleaner2, 30, 10, true, 3);
    }
}

// Helper to quickly make a constant or zero image
Eigen::VectorXd constant_image(int n, double value) {
    return Eigen::VectorXd::Constant(n, value);
}
Eigen::VectorXd zero_image(int n) {
    return Eigen::VectorXd::Zero(n);
}

// Helper to avoid repetition for Tailcuts tests
void run_tailcuts_clean_tests(const CameraGeometry &camera, int num_pixels) {
    SUBCASE("EMPTY_IMAGE") {
        auto clean_image = TailcutsCleaner::tailcuts_clean(camera, zero_image(num_pixels), 1, 1);
        CHECK(clean_image.array().sum() == 0);
    }
    SUBCASE("CONSTANT_IMAGE") {
        auto clean_image = TailcutsCleaner::tailcuts_clean(camera, constant_image(num_pixels, 10), 1, 1);
        CHECK(clean_image.array().cast<int>().sum() == num_pixels);
    }
    SUBCASE("SOLO_ABOVE_THRESHOLD") {
        auto solo_image = constant_image(num_pixels, 5);
        solo_image(10) = 10;
        auto clean_image = TailcutsCleaner::tailcuts_clean(camera, solo_image, 8, 1);
        CHECK(clean_image.array().cast<int>().sum() == 5);
        CHECK(clean_image(6) == 1);
        CHECK(clean_image(9) == 1);
        CHECK(clean_image(10) == 1);
        CHECK(clean_image(11) == 1);
        CHECK(clean_image(14) == 1);
    }
    SUBCASE("KEEP_ISOLATED_PIXELS") {
        auto img = constant_image(num_pixels, 1);
        img(10) = 10; img(6) = 5; img(9) = 5; img(0) = 10;
        auto clean_image = TailcutsCleaner::tailcuts_clean(camera, img, 8, 2, true);
        CHECK(clean_image.array().cast<int>().sum() == 4);
        CHECK(clean_image(6) == 1);
        CHECK(clean_image(9) == 1);
        CHECK(clean_image(10) == 1);
        CHECK(clean_image(0) == 1);
    }
    SUBCASE("NO_KEEP_ISOLATED_PIXELS") {
        auto img = constant_image(num_pixels, 1);
        img(10) = 10; img(6) = 10; img(9) = 10; img(0) = 10;
        auto clean_image = TailcutsCleaner::tailcuts_clean(camera, img, 8, 2, false, 2);
        CHECK(clean_image.array().cast<int>().sum() == 3);
        CHECK(clean_image(0) == 0);
        CHECK(clean_image(6) == 1);
        CHECK(clean_image(9) == 1);
        CHECK(clean_image(10) == 1);
    }
}

TEST_CASE("TEST_TAILCUTS_CLEAN") {
    int W = 4, H = 4, num_pixels = W * H;
    CameraGeometry camera = create_grid_camera(W, H);
    run_tailcuts_clean_tests(camera, num_pixels);
}

// Helper to prepare square test images (used for leakage, morphology, etc.)
template <typename F>
void with_square_camera(int w, F f) {
    int num_pixels = w * w;
    CameraGeometry camera = create_grid_camera(w, w);
    f(num_pixels, camera);
}

TEST_CASE("TEST_LEAKAGE_PARAMETERS") {
    with_square_camera(5, [](int num_pixels, const CameraGeometry &camera) {
        SUBCASE("EMPTY_IMAGE") {
            auto leakage_parameters =
                ImageProcessor::leakage_parameter(camera, zero_image(num_pixels));
            CHECK(std::isnan(leakage_parameters.pixels_width_1));
            CHECK(std::isnan(leakage_parameters.pixels_width_2));
            CHECK(std::isnan(leakage_parameters.intensity_width_1));
            CHECK(std::isnan(leakage_parameters.intensity_width_2));
        }
        SUBCASE("CONSTANT_IMAGE") {
            auto leakage_parameters = ImageProcessor::leakage_parameter(
                                        camera, constant_image(num_pixels, 10));
            CHECK(leakage_parameters.pixels_width_1 == 16 / 25.0);
            CHECK(leakage_parameters.pixels_width_2 == 24 / 25.0);
            CHECK(leakage_parameters.intensity_width_1 == 160 / 250.0);
            CHECK(leakage_parameters.intensity_width_2 == 240 / 250.0);
        }
        SUBCASE("NORMAL_IMAGE") {
            auto normal_image = constant_image(num_pixels, 1);
            normal_image(0) = 10;
            auto leakage_parameters =
                ImageProcessor::leakage_parameter(camera, normal_image);
            CHECK(leakage_parameters.pixels_width_1 == 16 / 25.0);
            CHECK(leakage_parameters.pixels_width_2 == 24 / 25.0);
            CHECK(leakage_parameters.intensity_width_1 == 1.0 * (15 + 10) / (24 + 10));
            CHECK(leakage_parameters.intensity_width_2 == 1.0 * (23 + 10) / (24 + 10));
        }
    });
}

TEST_CASE("TEST_MORPHOLOGY_PARAMETERS") {
    with_square_camera(5, [](int num_pixels, const CameraGeometry &camera) {
        SUBCASE("EMPTY_MASk") {
            Eigen::Vector<bool, -1> empty_mask =
                Eigen::Vector<bool, -1>::Zero(num_pixels);
            auto mp = ImageProcessor::morphology_parameter(camera, empty_mask);
            CHECK(mp.n_pixels == 0);
            CHECK(mp.n_small_islands == 0);
            CHECK(mp.n_medium_islands == 0);
            CHECK(mp.n_large_islands == 0);
        }
        SUBCASE("FULL_MASK") {
            Eigen::Vector<bool, -1> full_mask =
                Eigen::Vector<bool, -1>::Constant(num_pixels, true);
            auto mp = ImageProcessor::morphology_parameter(camera, full_mask);
            CHECK(mp.n_pixels == num_pixels);
            CHECK(mp.n_islands == 1);
            CHECK(mp.n_medium_islands == 1);
        }
        SUBCASE("NORMAL_MASK") {
            Eigen::Vector<bool, -1> mask = Eigen::Vector<bool, -1>::Zero(num_pixels);
            for(int i=0; i<5; ++i) mask(i) = mask(20+i) = true;
            auto mp = ImageProcessor::morphology_parameter(camera, mask);
            CHECK(mp.n_pixels == 10);
            CHECK(mp.n_islands == 2);
            CHECK(mp.n_small_islands == 2);
            CHECK(mp.n_medium_islands == 0);
            CHECK(mp.n_large_islands == 0);
        }
    });
}

TEST_CASE("TEST_HILLAS_PARAMETER") {
    with_square_camera(4, [](int num_pixels, const CameraGeometry &camera) {
        SUBCASE("ONLY_DIAGONAL_PIXELS") {
            Eigen::VectorXd diagonal_image = zero_image(num_pixels);
            diagonal_image(0) = 1;
            diagonal_image(5) = 1;
            diagonal_image(10) = 1;
            diagonal_image(15) = 1;
            auto hillas = ImageProcessor::hillas_parameter(camera, diagonal_image);
            CHECK(hillas.psi == doctest::Approx(M_PI / 4));
            CHECK(hillas.intensity == 4);
            CHECK(hillas.x == doctest::Approx(1.5));
            CHECK(hillas.y == doctest::Approx(1.5));
        }
    });
}

TEST_CASE("TEST_DILATE_IMAGE") {
    with_square_camera(4, [](int num_pixels, const CameraGeometry &camera) {
        Eigen::Vector<bool, -1> mask = Eigen::Vector<bool, -1>::Zero(num_pixels);
        mask(0) = true;
        ImageProcessor::dilate_image(camera, mask);
        CHECK(mask.array().cast<int>().sum() == 3);
        CHECK(mask(1));
        CHECK(mask(4));
        CHECK(mask(0));
    });
}

TEST_CASE("TEST_CUT_RADIUS") {
    with_square_camera(4, [](int num_pixels, const CameraGeometry &camera) {
        auto pixel_mask = ImageProcessor::cut_pixel_distance(camera, 1, 2 * 180 / M_PI);
        CHECK(pixel_mask.array().cast<int>().sum() == 6);
        auto pixel_mask2 = ImageProcessor::cut_pixel_distance(camera, 1, 1 * 180 / M_PI);
        CHECK(pixel_mask2.array().cast<int>().sum() == 3);
    });
}