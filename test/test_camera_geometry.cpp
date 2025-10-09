#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "CameraGeometry.hh"
#include "doctest/doctest.h"


TEST_CASE("test_neighbor_matrix")
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
    CHECK(camera.neigh_matrix.row(5).sum() == 4);  // not includes self due to KNN search
    
    // Check specific neighbors for center pixel
    CHECK(camera.neigh_matrix.coeff(5, 1) == 1);  // left neighbor
    CHECK(camera.neigh_matrix.coeff(5, 9) == 1);  // right neighbor
    CHECK(camera.neigh_matrix.coeff(5, 4) == 1);  // top neighbor 
    CHECK(camera.neigh_matrix.coeff(5, 6) == 1);  // bottom neighbor

    // Test corner pixel (0) should have 2 neighbors
    CHECK(camera.neigh_matrix.row(0).sum() == 2);  // not includes self
    
    // Check specific neighbors for corner pixel
    CHECK(camera.neigh_matrix.coeff(0, 1) == 1);  // right neighbor
    CHECK(camera.neigh_matrix.coeff(0, 4) == 1);  // bottom neighbor

    // Test edge pixel (2) should have 3 neighbors
    CHECK(camera.neigh_matrix.row(2).sum() == 3);  // not includes self
    // Check specific neighbors for edge pixel
    CHECK(camera.neigh_matrix.coeff(2, 1) == 1);  // left neighbor
    CHECK(camera.neigh_matrix.coeff(2, 3) == 1);  // right neighbor
    CHECK(camera.neigh_matrix.coeff(2, 6) == 1);  // bottom neighbor
}
TEST_CASE("test_get_border_pixel_mask")
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
    auto border_mask = camera.get_border_pixel_mask(1);
    CHECK(border_mask.size() == 25);
    CHECK(border_mask.count() == 16);
    CHECK(border_mask(5) == true);
    CHECK(border_mask(6) == false);
    CHECK(border_mask(9) == true);
    CHECK(border_mask(10) == true);
    CHECK(border_mask(13) == false);
    auto border_mask_2 = camera.get_border_pixel_mask(2);
    CHECK(border_mask_2.count() == 24);
    
}
