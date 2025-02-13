#include "ImageProcessor.hh"
#include "CameraGeometry.hh"
#include "ImageParameters.hh"
#include "spdlog/spdlog.h"


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
// First is clean the image , then extractor the parameter
void ImageProcessor::operator()(ArrayEvent& event)
{
    if(!event.dl1)
    {
        event.dl1 = DL1Event();
    }
    for(const auto& [tel_id, dl0_camera]: event.dl0->tels)
    {
        DL1Camera dl1_camera;
        // Mask for the image 
        auto image_mask = (*image_cleaner)(subarray.tels.at(tel_id).camera_description.camera_geometry, dl0_camera->image);
        Eigen::VectorXd masked_image = image_mask.select(dl0_camera->image, Eigen::VectorXd::Zero(dl0_camera->image.size()));
        HillasParameter hillas_parameter = ImageProcessor::hillas_parameter(subarray.tels.at(tel_id).camera_description.camera_geometry, masked_image);
        LeakageParameter leakage_parameter = ImageProcessor::leakage_parameter(const_cast<CameraGeometry&>(subarray.tels.at(tel_id).camera_description.camera_geometry), masked_image);
        // Tempory image are copyed from dl0_camera
        dl1_camera.image = dl0_camera->image;
        dl1_camera.peak_time = dl0_camera->peak_time;
        dl1_camera.mask = std::move(image_mask);  // Image mask is not used in the future
        dl1_camera.image_parameters.hillas = hillas_parameter;
        dl1_camera.image_parameters.leakage = leakage_parameter;
        event.dl1->add_tel(tel_id, std::move(dl1_camera));
    }

}
HillasParameter ImageProcessor::hillas_parameter(const CameraGeometry& camera_geometry, const Eigen::VectorXd& masked_image)
{
    // Use the mask to get the image
    double intensity = masked_image.sum();
    Eigen::MatrixXd cov_matrix{2, 2};
    double x = camera_geometry.pix_x_fov.dot(masked_image)/intensity;
    double y = camera_geometry.pix_y_fov.dot(masked_image)/intensity;
    double r = std::sqrt(x*x + y*y);
    double phi = std::atan2(y, x);
    Eigen::VectorXd delta_x = camera_geometry.pix_x_fov.array() - x;
    Eigen::VectorXd delta_y = camera_geometry.pix_y_fov.array() - y;
    cov_matrix(0, 0) = (delta_x.array() * delta_x.array()).matrix().dot(masked_image)/(intensity - 1);
    cov_matrix(1, 1) = (delta_y.array() * delta_y.array()).matrix().dot(masked_image)/(intensity - 1);
    cov_matrix(0, 1) = (delta_x.array() * delta_y.array()).matrix().dot(masked_image)/(intensity - 1);
    cov_matrix(1, 0) = cov_matrix(0, 1);
    double length,width, psi = 0;
    double skewness = 0, kurtosis = 0;
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigensolver(cov_matrix);
    if(eigensolver.info() != Eigen::Success) {
        spdlog::warn("Eigenvalue decomposition failed");
        psi = length = width = skewness = kurtosis = std::numeric_limits<double>::quiet_NaN();
    }
    else
    {
        Eigen::VectorXd eigen_values = eigensolver.eigenvalues();
        Eigen::MatrixXd eigen_vectors = eigensolver.eigenvectors();
        length = std::sqrt(eigen_values(0));
        width = std::sqrt(eigen_values(1));
        if(eigen_vectors(0, 0) != 0)
        {
            psi = std::atan2(eigen_vectors(1, 0), eigen_vectors(0, 0));
        }
        else
        {
            psi = M_PI/2;
        }
    }
    // Uint vector along the major axis is (cos(psi), sin(psi))
    Eigen::VectorXd longitudinal = delta_x.array() * std::cos(psi) + delta_y.array() * std::sin(psi);
    double m3_long = pow(longitudinal.array(), 3).matrix().dot(masked_image);
    double m4_long = pow(longitudinal.array(), 4).matrix().dot(masked_image);
    skewness = m3_long/pow(length, 3);
    kurtosis = m4_long/pow(length, 4);
    return HillasParameter{length, width, psi, x, y, skewness, kurtosis, intensity, r, phi};

}
LeakageParameter ImageProcessor::leakage_parameter(CameraGeometry& camera_geometry, const Eigen::VectorXd& masked_image)
{
    // Use the mask to get the image
    auto  outermost_pixel_mask = camera_geometry.get_border_pixel_mask(1);
    auto  second_outermost_pixel_mask = camera_geometry.get_border_pixel_mask(2);
    int   image_pixels = (masked_image.array() > 0).count();
    double intensity = masked_image.sum();
    double intensity_width_1 = outermost_pixel_mask.cast<double>().dot(masked_image)/intensity;
    double intensity_width_2 = second_outermost_pixel_mask.cast<double>().dot(masked_image)/intensity;
    double pixel_width_1 = 1.0 * (outermost_pixel_mask.array() || (masked_image.array() > 0)).count() / image_pixels;
    double pixel_width_2 = 1.0 * (second_outermost_pixel_mask.array() || (masked_image.array() > 0)).count()/ image_pixels;
    return LeakageParameter{intensity_width_1, intensity_width_2, pixel_width_1, pixel_width_2};

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