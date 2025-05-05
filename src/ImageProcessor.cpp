#include "ImageProcessor.hh"
#include "CameraGeometry.hh"
#include "Eigen/Dense"
#include "ImageParameters.hh"
#include "spdlog/spdlog.h"
#include <queue>
#include <iostream>

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
    Eigen::Vector<bool, -1> pixel_with_picture_neighbors = (camera_geometry.neigh_matrix * pixel_in_picture.cast<int>()).array() > 0 ;
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
    std::cout << "image_cleaner_type: " << image_cleaner_type << std::endl;
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
        if(masked_image.sum() < 50)
        {
            continue;
        }
        HillasParameter hillas_parameter = ImageProcessor::hillas_parameter(subarray.tels.at(tel_id).camera_description.camera_geometry, masked_image);
        LeakageParameter leakage_parameter = ImageProcessor::leakage_parameter(const_cast<CameraGeometry&>(subarray.tels.at(tel_id).camera_description.camera_geometry), masked_image);
        ConcentrationParameter concentration_parameter = ImageProcessor::concentration_parameter(subarray.tels.at(tel_id).camera_description.camera_geometry, masked_image, hillas_parameter);
        MorphologyParameter morphology_parameter = ImageProcessor::morphology_parameter(subarray.tels.at(tel_id).camera_description.camera_geometry, image_mask);
        IntensityParameter intensity_parameter = ImageProcessor::intensity_parameter(masked_image);
        // Tempory image are copyed from dl0_camera
        dl1_camera.image = dl0_camera->image.cast<float>();
        dl1_camera.peak_time = dl0_camera->peak_time.cast<float>();
        dl1_camera.mask = std::move(image_mask);  // Image mask is not used in the future
        dl1_camera.image_parameters.hillas = hillas_parameter;
        dl1_camera.image_parameters.leakage = leakage_parameter;
        dl1_camera.image_parameters.concentration = concentration_parameter;
        dl1_camera.image_parameters.morphology = morphology_parameter;
        dl1_camera.image_parameters.intensity = intensity_parameter;
        event.dl1->add_tel(tel_id, std::move(dl1_camera));
    }

}
// TODO: Add the unit test for the hillas parameter
HillasParameter ImageProcessor::hillas_parameter(const CameraGeometry& camera_geometry, const Eigen::VectorXd& masked_image)
{
    // Use the mask to get the image
    double intensity = masked_image.sum();
    Eigen::MatrixXd cov_matrix{2, 2};
    double x = camera_geometry.get_pix_x_fov().dot(masked_image)/intensity;
    double y = camera_geometry.get_pix_y_fov().dot(masked_image)/intensity;
    double r = std::sqrt(x*x + y*y);
    double phi = std::atan2(y, x);
    Eigen::VectorXd delta_x = camera_geometry.get_pix_x_fov().array() - x;
    Eigen::VectorXd delta_y = camera_geometry.get_pix_y_fov().array() - y;
    cov_matrix(0, 0) = (delta_x.array().square() * masked_image.array()).sum()/(intensity - 1);
    cov_matrix(1, 1) = (delta_y.array().square() * masked_image.array()).sum()/(intensity - 1);
    cov_matrix(0, 1) = (delta_x.array() * delta_y.array() * masked_image.array()).sum()/(intensity - 1);
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
        length = std::sqrt(eigen_values(1));
        width = std::sqrt(eigen_values(0));
        if(eigen_vectors.col(1)[0] != 0)
        {
            psi = std::atan2(eigen_vectors.col(1)[1], eigen_vectors.col(1)[0]);
        }
        else
        {
            psi = M_PI/2;
        }
    }
    // Uint vector along the major axis is (cos(psi), sin(psi))
    Eigen::VectorXd longitudinal = delta_x.array() * std::cos(psi) + delta_y.array() * std::sin(psi);
    double m3_long = pow(longitudinal.array(), 3).matrix().dot(masked_image)/intensity;
    double m4_long = pow(longitudinal.array(), 4).matrix().dot(masked_image)/intensity;
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
    double pixel_width_1 = 1.0 * (outermost_pixel_mask.array() && (masked_image.array() > 0)).count() / image_pixels;
    double pixel_width_2 = 1.0 * (second_outermost_pixel_mask.array() && (masked_image.array() > 0)).count()/ image_pixels;
    return LeakageParameter{pixel_width_1, pixel_width_2, intensity_width_1, intensity_width_2};

}
// TODO: Add the unit test for the concentration parameter
ConcentrationParameter ImageProcessor::concentration_parameter(const CameraGeometry& camera_geometry, const Eigen::VectorXd& masked_image, const HillasParameter& hillas_parameter)
{
    double concentration_pixel = masked_image.maxCoeff()/ hillas_parameter.intensity;
    auto delta_x = camera_geometry.pix_x_fov.array() - hillas_parameter.x;
    auto delta_y = camera_geometry.pix_y_fov.array() - hillas_parameter.y;
    Eigen::ArrayXd distance = (delta_x.array() * delta_x.array() + delta_y.array() * delta_y.array()).sqrt();
    auto mask_cog = distance < camera_geometry.pix_width_fov[0];
    double concentration_cog = masked_image.dot(mask_cog.cast<double>().matrix()) / hillas_parameter.intensity;

    // Rotate the axis anti-clockwise by the psi angle
    Eigen::Matrix2d rotation_matrix = (Eigen::Matrix2d() << cos(hillas_parameter.psi), sin(hillas_parameter.psi), -sin(hillas_parameter.psi), cos(hillas_parameter.psi)).finished();
    Eigen::ArrayXd delta_x_rotated = rotation_matrix.row(0)(0) * delta_x.array() + rotation_matrix.row(0)(1) * delta_y.array();
    Eigen::ArrayXd delta_y_rotated = rotation_matrix.row(1)(0) * delta_x.array() + rotation_matrix.row(1)(1) * delta_y.array();
    auto mask_core = (delta_x_rotated.array() * delta_x_rotated.array() / pow(hillas_parameter.length, 2) + delta_y_rotated.array() * delta_y_rotated.array() / pow(hillas_parameter.width, 2)) < 1;
    double concentration_core = masked_image.dot(mask_core.cast<double>().matrix()) / hillas_parameter.intensity;
    return ConcentrationParameter{concentration_cog, concentration_core, concentration_pixel};
}
MorphologyParameter ImageProcessor::morphology_parameter(const CameraGeometry& camera_geometry, const Eigen::Vector<bool, -1>& image_mask)
{
    std::unordered_map<size_t, std::vector<size_t>> island_map;
    Eigen::Vector<bool, -1> pixel_in_island = Eigen::Vector<bool, -1>::Zero(image_mask.size());
    size_t island_id = 0;
    for(size_t i = 0; i < image_mask.size(); ++i)
    {
        std::queue<size_t> queue;
        if(image_mask[i] && !pixel_in_island[i])
        {
            queue.push(i);
            pixel_in_island[i] = true;
            island_map[island_id].push_back(i);
            while(!queue.empty())
            {
                auto pixel = queue.front();
                queue.pop();
                for(Eigen::SparseMatrix<int, Eigen::RowMajor>::InnerIterator it(camera_geometry.neigh_matrix, pixel); it; ++it)
                {
                    if(it.value() > 0 && !pixel_in_island[it.col()] && image_mask[it.col()])
                    {
                        queue.push(it.col());
                        pixel_in_island[it.col()] = true;
                        island_map[island_id].push_back(it.col());
                    }
                }
            }
            island_id++;
        }
    }
    int num_island = island_map.size();
    int n_pixels = image_mask.count();
    int n_small_islands = 0;
    int n_medium_islands = 0;
    int n_large_islands = 0;
    for(const auto& [island_id, island_pixels]: island_map)
    {
        if(island_pixels.size() < 10)
        {
            n_small_islands++;
        }
        else if(island_pixels.size() < 30)
        {
            n_medium_islands++;
        }
        else
        {
            n_large_islands++;
        }
    }
    return MorphologyParameter{n_pixels, num_island, n_small_islands, n_medium_islands, n_large_islands};

}
IntensityParameter ImageProcessor::intensity_parameter(const Eigen::VectorXd& masked_image)
{
    double intensity_max = masked_image.maxCoeff();
    double intensity_mean = masked_image.sum()/masked_image.count();
    double intensity_std = 0;
    for(auto ipe: masked_image)
    {
        if(ipe > 0)
        {
            intensity_std += std::pow(ipe - intensity_mean, 2);
        }
    }
    intensity_std = std::sqrt(intensity_std/masked_image.count());
    return IntensityParameter{intensity_max, intensity_mean, intensity_std};
}

void ImageProcessor::dilate_image(const CameraGeometry& camera_geometry, Eigen::Vector<bool, -1>& image_mask)
{
    Eigen::SparseMatrix<int, Eigen::RowMajor> dilated_matrix = camera_geometry.neigh_matrix;
    image_mask = (dilated_matrix * image_mask.cast<int>().matrix()).cast<bool>() || image_mask;
}

json ImageProcessor::get_default_config()
{
    std::string default_config = R"(
    {
        "image_cleaner_type": "Tailcuts_cleaner"
    }
    )";
    json base_config = Configurable::from_string(default_config);
    base_config["Tailcuts_cleaner"] = TailcutsCleaner::get_default_config();
    return base_config;
}