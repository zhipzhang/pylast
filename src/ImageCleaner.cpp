#include "ImageCleaner.hh"
#include "spdlog/spdlog.h"
json TailcutsCleaner::default_config() const
{
    return get_default_config();
}
json TailcutsCleaner::get_default_config()
{
    return Configurable::from_string(R"(
    {
        "picture_thresh": 10,
        "boundary_thresh": 5,
        "keep_isolated_pixels": false,
        "min_number_picture_neighbors": 2
    }
    )");
}

void TailcutsCleaner::configure(const json& config)
{
    try {
        const json& cfg = config.contains("Tailcuts_cleaner") ? config.at("Tailcuts_cleaner") : config;
        
        if (config.contains("Tailcuts_cleaner")) {
            spdlog::debug("Using top level TailcutsCleaner config");
        }
        picture_thresh = cfg.at("picture_thresh");
        boundary_thresh = cfg.at("boundary_thresh"); 
        keep_isolated_pixels = cfg.at("keep_isolated_pixels");
        min_number_picture_neighbors = cfg.at("min_number_picture_neighbors");
    }
    catch(const std::exception& e) {
        throw std::runtime_error("Error configuring TailcutsCleaner: " + std::string(e.what()));
    }
}

Eigen::Vector<bool, -1> TailcutsCleaner::operator()(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image) const
{
    return tailcuts_clean(camera_geometry, image, picture_thresh, boundary_thresh, keep_isolated_pixels, min_number_picture_neighbors);
}

Eigen::Vector<bool, -1> TailcutsCleaner::tailcuts_clean(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image, double picture_thresh, double boundary_thresh, bool keep_isolated_pixels, int min_number_picture_neighbors)
{
    Eigen::Vector<bool, -1> pixel_above_picture = (image.array() >= picture_thresh);
    Eigen::Vector<bool, -1> pixel_in_picture;
    if(keep_isolated_pixels or min_number_picture_neighbors == 0)
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