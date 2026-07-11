#include "ImageCleaner.hh"
#include "Eigen/src/Core/MathFunctions.h"
#include "spdlog/spdlog.h"

json TailcutsCleaner::get_default_config()
{
    json config;
    config["picture_thresh"] = 10;
    config["boundary_thresh"] = 5;
    config["keep_isolated_pixels"] = false;
    config["min_number_picture_neighbors"] = 2;
    return config;
}

void TailcutsCleaner::registerParams()
{
    registerParam<double>("picture_thresh",10, picture_thresh);
    registerParam<double>("boundary_thresh",5, boundary_thresh);
    registerParam<bool>("keep_isolated_pixels", false, keep_isolated_pixels);
    registerParam<int>("min_number_picture_neighbors", 2, min_number_picture_neighbors);
}

void TailcutsCleaner::setUp()
{
}

Eigen::Vector<bool, -1> TailcutsCleaner::operator()(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image, double level) const
{
    return tailcuts_clean(camera_geometry, image, picture_thresh * level, boundary_thresh * level, keep_isolated_pixels, min_number_picture_neighbors);
}

Eigen::Vector<bool, -1> TailcutsCleaner::tailcuts_clean(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image, double picture_thresh, double boundary_thresh, bool keep_isolated_pixels, int min_number_picture_neighbors)
{
    Eigen::Vector<bool, -1> pixel_above_picture = (image.array() >= picture_thresh);
    Eigen::Vector<bool, -1> pixel_in_picture;
    if(keep_isolated_pixels || min_number_picture_neighbors == 0)
    {
        pixel_in_picture = pixel_above_picture;
    }
    else 
    {
        Eigen::VectorXi num_neighbors_above_picture = camera_geometry.neigh_matrix * pixel_above_picture.cast<int>();
        Eigen::Vector<bool, -1> have_enough_neighbors = (num_neighbors_above_picture.array() >= min_number_picture_neighbors);
        pixel_in_picture = (pixel_above_picture.array() && have_enough_neighbors.array()).matrix();
    }
    Eigen::Vector<bool, -1> pixel_above_boundary = (image.array() >= boundary_thresh);
    Eigen::Vector<bool, -1> pixel_with_picture_neighbors = (camera_geometry.neigh_matrix * pixel_in_picture.cast<int>()).array() > 0;
    if(keep_isolated_pixels)
    {
        return (pixel_above_boundary.array() && pixel_with_picture_neighbors.array()) || pixel_in_picture.array();
    }
    else 
    {
        Eigen::Vector<bool, -1> pixel_with_boundary_neighbors = (camera_geometry.neigh_matrix * pixel_above_boundary.cast<int>()).array() > 0;
        return (pixel_above_boundary.array() && pixel_with_picture_neighbors.array()) || (pixel_in_picture.array() && pixel_with_boundary_neighbors.array());
    }
}

//dilation 膨胀，可以扩一圈，增加低能图像完整性
Eigen::Vector<bool, -1> dilate(const CameraGeometry& camera_geometry,
                               const Eigen::Vector<bool, -1>& mask)
{
    Eigen::VectorXi neigh_true_count = camera_geometry.neigh_matrix * mask.cast<int>();
    Eigen::Vector<bool, -1> has_true_neighbor = (neigh_true_count.array() > 0).matrix();
    return (mask.array() || has_true_neighbor.array()).matrix();
}

// 自适应清理
Eigen::Vector<bool, -1> TailcutsCleaner::tailcuts_clean2(
    const CameraGeometry& camera_geometry,
    const Eigen::VectorXd& image) const
{

    const double maxval = (image.size() > 0) ? image.maxCoeff() : 0.0;

    const double picture_auto  = std::max(10.0, maxval / 10.0);
    const double boundary_auto = std::max(5.0,  maxval / 20.0);

    return tailcuts_clean(
        camera_geometry,
        image,
        picture_auto,
        boundary_auto,
        keep_isolated_pixels,
        min_number_picture_neighbors
    );
}
