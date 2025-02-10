#include "ImageProcessor.hh"



Eigen::Vector<bool, -1> ImageProcessor::tailcuts_clean(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image, double picture_thresh, double boundary_thresh, bool keep_isolated_pixels, int min_number_picture_neighbors)
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
void ImageProcessor::configure(const json& config)
{
    image_cleaner_type = config["image_cleaner_type"];
    if(image_cleaner_type == "Tailcuts_cleaner")
    {
        image_cleaner = std::make_unique<TailcutsCleaner>(config["Tailcuts_cleaner"]);
    }
}
json ImageProcessor::get_default_config()
{
    std::stringstream ss;
    ss << R"(
    {
        "image_cleaner_type": "Tailcuts_cleaner",
    }
    )";
    json base_config = Configurable::from_string(ss.str());
    base_config["Tailcuts_cleaner"] = TailcutsCleaner::get_default_config();
    return base_config;
}