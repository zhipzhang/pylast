#include "ImageProcessor.hh"
#include "ArrayEvent.hh"
#include "CameraGeometry.hh"
#include "Eigen/Dense"
#include "Eigen/src/Core/Matrix.h"
#include "Gaussian2DFunctor.hh"
#include "ImageParameters.hh"
#include "SubarrayDescription.hh"
#include "spdlog/spdlog.h"
#include <cmath>
#include <iostream>
#include <queue>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());

Eigen::Vector<bool, -1> ImageProcessor::tailcuts_clean(
    const CameraGeometry &camera_geometry, const Eigen::VectorXd &image,
    double picture_thresh, double boundary_thresh, bool keep_isolated_pixels,
    int min_number_picture_neighbors) {
  Eigen::Vector<bool, -1> pixel_above_picture =
      (image.array() >= picture_thresh);
  Eigen::Vector<bool, -1> pixel_in_picture;
  if (keep_isolated_pixels || min_number_picture_neighbors == 0) {
    pixel_in_picture = pixel_above_picture;
  } else {
    Eigen::VectorXi num_neighbors_above_picture =
        camera_geometry.neigh_matrix * pixel_above_picture.cast<int>();
    Eigen::Vector<bool, -1> have_enough_neighbors =
        (num_neighbors_above_picture.array() >= min_number_picture_neighbors);
    pixel_in_picture =
        (pixel_above_picture.array() && have_enough_neighbors.array()).matrix();
  }
  Eigen::Vector<bool, -1> pixel_above_boundary =
      (image.array() >= boundary_thresh);
  Eigen::Vector<bool, -1> pixel_with_picture_neighbors =
      (camera_geometry.neigh_matrix * pixel_in_picture.cast<int>()).array() > 0;
  if (keep_isolated_pixels) {
    return (pixel_above_boundary.array() &&
            pixel_with_picture_neighbors.array()) ||
           pixel_in_picture.array();
  } else {
    Eigen::Vector<bool, -1> pixel_with_boundary_neighbors =
        (camera_geometry.neigh_matrix * pixel_above_boundary.cast<int>())
            .array() > 0;
    return (pixel_above_boundary.array() &&
            pixel_with_picture_neighbors.array()) ||
           (pixel_in_picture.array() && pixel_with_boundary_neighbors.array());
  }
}
void ImageProcessor::registerParams() {
  registerParam<std::string>("image_cleaner_type", "Tailcuts_cleaner",
                             image_cleaner_type);
  registerParam<double>("poisson_noise", 0.0, poisson_noise);
  registerParam<double>("cut_radius", 0.0, cut_radius);
  registerParam<int>("trigger_pixels", 4, fake_trigger_pixels);
  registerParam<double>("trigger_pe", 8, fake_trigger_pe);
  registerParam<bool>("use_random_gaussian", false, use_random_gaussian);
  registerParam<double>("random_gaussian_level", 0, random_gaussian_level);
  registerParam<bool>("only_use_largerst_island", false,
                      only_use_largerst_island);
}

Eigen::Vector<bool, -1>
ImageProcessor::image_clean(const CameraGeometry &camera_geometry,
                            const Eigen::VectorXd &image) {
  return (*image_cleaner)(camera_geometry, image);
}
void ImageProcessor::setUp() {
  std::cout << "image_cleaner_type: " << image_cleaner_type << std::endl;
  if (image_cleaner_type == "Tailcuts_cleaner") {
    if (getConfig().contains("Tailcuts_cleaner"))
      image_cleaner =
          std::make_unique<TailcutsCleaner>(getConfig()["Tailcuts_cleaner"]);
    else
      image_cleaner = std::make_unique<TailcutsCleaner>();
  }
  if (cut_radius > 0.0) {
    use_cut_radius = true;
  }
}
// First is clean the image , then extractor the parameter
void ImageProcessor::operator()(ArrayEvent &event) {
  event.rounded_tel_hillas.clear();
  if (!event.dl1) {
    event.dl1 = DL1Event();
  }
  for (const auto &[tel_id, dl0_camera] : event.dl0->tels) {
    DL1Camera dl1_camera;
    // Mask for the image
    auto image_mask = (*image_cleaner)(
        subarray.tels.at(tel_id).camera_description.camera_geometry,
        dl0_camera->image);
    if (use_cut_radius) {
      auto masked_radius = ImageProcessor::cut_pixel_distance(
          subarray.tels.at(tel_id).camera_description.camera_geometry,
          subarray.tels.at(tel_id).optics_description.equivalent_focal_length,
          cut_radius);
      image_mask = image_mask.array() && masked_radius.array();
    }
    MorphologyParameter morphology_parameter =
        ImageProcessor::morphology_parameter(
            subarray.tels.at(tel_id).camera_description.camera_geometry,
            image_mask, only_use_largerst_island);

    Eigen::VectorXd masked_image = image_mask.select(
        dl0_camera->image, Eigen::VectorXd::Zero(dl0_camera->image.size()));
    if (masked_image.sum() < 50) {
      continue;
    }
    HillasParameter hillas_parameter = ImageProcessor::hillas_parameter(
        subarray.tels.at(tel_id).camera_description.camera_geometry,
        masked_image);
    LeakageParameter leakage_parameter = ImageProcessor::leakage_parameter(
        const_cast<CameraGeometry &>(
            subarray.tels.at(tel_id).camera_description.camera_geometry),
        masked_image);
    ConcentrationParameter concentration_parameter =
        ImageProcessor::concentration_parameter(
            subarray.tels.at(tel_id).camera_description.camera_geometry,
            masked_image, hillas_parameter);
    IntensityParameter intensity_parameter =
        ImageProcessor::intensity_parameter(masked_image);
    TwoGaussianFitResult two_gaussian_fit;
    //TwoGaussianFitResult two_gaussian_fit = ImageProcessor::two_gaussian_fit(
    //    subarray.tels.at(tel_id).camera_description.camera_geometry,
     //   masked_image, image_mask, hillas_parameter);
    // Tempory image are copyed from dl0_camera
    Eigen::VectorXf image = dl0_camera->image.cast<float>();
    Eigen::VectorXf peak_time = dl0_camera->peak_time.cast<float>();
    Eigen::Vector<bool, -1> mask = std::move(image_mask);
    event.dl1->add_tel(
        tel_id, DL1Camera{.image_parameters =
                              ImageParameters{
                                  hillas_parameter, two_gaussian_fit, leakage_parameter,
                                  concentration_parameter, morphology_parameter,
                                  intensity_parameter},
                          .image = std::move(image),
                          .peak_time = std::move(peak_time),
                          .mask = std::move(mask)});
  }

  handle_simulation_level(event);
}
// TODO: Add the unit test for the hillas parameter
HillasParameter
ImageProcessor::hillas_parameter(const CameraGeometry &camera_geometry,
                                 const Eigen::VectorXd &masked_image) {
  // Use the mask to get the image
  double intensity = masked_image.sum();
  Eigen::MatrixXd cov_matrix{2, 2};
  double x = camera_geometry.get_pix_x_fov().dot(masked_image) / intensity;
  double y = camera_geometry.get_pix_y_fov().dot(masked_image) / intensity;
  double r = std::sqrt(x * x + y * y);
  double phi = std::atan2(y, x);
  Eigen::VectorXd delta_x = camera_geometry.get_pix_x_fov().array() - x;
  Eigen::VectorXd delta_y = camera_geometry.get_pix_y_fov().array() - y;
  cov_matrix(0, 0) =
      (delta_x.array().square() * masked_image.array()).sum() / (intensity - 1);
  cov_matrix(1, 1) =
      (delta_y.array().square() * masked_image.array()).sum() / (intensity - 1);
  cov_matrix(0, 1) =
      (delta_x.array() * delta_y.array() * masked_image.array()).sum() /
      (intensity - 1);
  cov_matrix(1, 0) = cov_matrix(0, 1);
  double length, width, psi = 0;
  double skewness = 0, kurtosis = 0;
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigensolver(cov_matrix);
  if (eigensolver.info() != Eigen::Success) {
    spdlog::warn("Eigenvalue decomposition failed");
    psi = length = width = skewness = kurtosis =
        std::numeric_limits<double>::quiet_NaN();
  } else {
    Eigen::VectorXd eigen_values = eigensolver.eigenvalues();
    Eigen::MatrixXd eigen_vectors = eigensolver.eigenvectors();
    length = std::sqrt(eigen_values(1));
    width = std::sqrt(eigen_values(0));
    if (eigen_vectors.col(1)[0] != 0) {
      psi = std::atan(eigen_vectors.col(1)[1]/ eigen_vectors.col(1)[0]);
    } else {
      psi = M_PI / 2;
    }
  }
  // Uint vector along the major axis is (cos(psi), sin(psi))
  Eigen::VectorXd longitudinal =
      delta_x.array() * std::cos(psi) + delta_y.array() * std::sin(psi);
  double m3_long =
      pow(longitudinal.array(), 3).matrix().dot(masked_image) / intensity;
  double m4_long =
      pow(longitudinal.array(), 4).matrix().dot(masked_image) / intensity;
  skewness = m3_long / pow(length, 3);
  kurtosis = m4_long / pow(length, 4);

  return HillasParameter{length,   width,    psi,       x, y,
                         skewness, kurtosis, intensity, r, phi};
}
TwoGaussianFitResult
ImageProcessor::two_gaussian_fit(const CameraGeometry &camera_geometry,
                                 const Eigen::VectorXd &image,
                                 const Eigen::Vector<bool, -1> &image_mask,
                                 const HillasParameter &hillas_parameter) {
      double pixel_size_fov = camera_geometry.get_pix_width_fov()[0];
      double initial_x = hillas_parameter.x;
      double initial_y = hillas_parameter.y;
      double initial_length = hillas_parameter.length;
      double initial_width = hillas_parameter.width;
      double initial_psi = hillas_parameter.psi;
      double initial_amplitude = image.maxCoeff();

      int num_image_pixels = image_mask.count();
      Eigen::VectorXd pix_x_in_image(num_image_pixels), pix_y_in_image(num_image_pixels), pix_pe_in_image(num_image_pixels);
      int index = 0;
      for(int i = 0; i < image_mask.size(); ++i)
      {
        if(image_mask[i])
        {
          pix_x_in_image[index] = camera_geometry.pix_x_fov[i];
          pix_y_in_image[index] = camera_geometry.pix_y_fov[i];
          pix_pe_in_image[index] = image[i];
          index++;
        }
      }
      Eigen::VectorXd initial_parameters(6);
      initial_parameters << initial_amplitude, initial_x, initial_y, initial_length, initial_width, initial_psi;
      Gaussian2DFunctor functor(pix_x_in_image, pix_y_in_image, pix_pe_in_image);
      Eigen::NumericalDiff<Gaussian2DFunctor> numDiff(functor);
      Eigen::LevenbergMarquardt<Eigen::NumericalDiff<Gaussian2DFunctor>> lm(numDiff);

      lm.setMaxfev(10000);
      lm.setFtol(1e-5);
      lm.setXtol(1e-5);
      lm.setFactor(20);

      Eigen::LevenbergMarquardtSpace::Status status = lm.minimize(initial_parameters);
      TwoGaussianFitResult results;
      results.chi2 = lm.fvec().squaredNorm();
      results.status = static_cast<int>(status);
      bool success = (status == Eigen::LevenbergMarquardtSpace::RelativeErrorAndReductionTooSmall || status == Eigen::LevenbergMarquardtSpace::RelativeErrorTooSmall || status == Eigen::LevenbergMarquardtSpace::RelativeReductionTooSmall);
      if(status == Eigen::LevenbergMarquardtSpace::TooManyFunctionEvaluation)
      {
        spdlog::warn("Too many function evaluation");
      }
      if(success)
      {
        results.converged = true;
        results.amplitude = initial_parameters[0];
        results.mean_x = initial_parameters[1];
        results.mean_y = initial_parameters[2];
        results.length = initial_parameters[3];
        results.width = initial_parameters[4];
        results.psi = initial_parameters[5];
        results.fit_size = results.amplitude/(pixel_size_fov*pixel_size_fov) * 2 * M_PI * results.length * results.width;
        if(results.psi < -M_PI/2)
        {
          results.psi += M_PI;
        }
        if(results.psi > M_PI/2)
        {
          results.psi -= M_PI;
        }
      }
      else
      {
        results.converged = false;
      }
      return results;
}
LeakageParameter
ImageProcessor::leakage_parameter(CameraGeometry &camera_geometry,
                                  const Eigen::VectorXd &masked_image) {
  // Use the mask to get the image
  auto outermost_pixel_mask = camera_geometry.get_border_pixel_mask(1);
  auto second_outermost_pixel_mask = camera_geometry.get_border_pixel_mask(2);
  int image_pixels = (masked_image.array() > 0).count();
  double intensity = masked_image.sum();
  if (intensity <= 0) {
    return LeakageParameter{std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN()};
  }
  double intensity_width_1 =
      outermost_pixel_mask.cast<double>().dot(masked_image) / intensity;
  double intensity_width_2 =
      second_outermost_pixel_mask.cast<double>().dot(masked_image) / intensity;
  double pixel_width_1 =
      1.0 *
      (outermost_pixel_mask.array() && (masked_image.array() > 0)).count() /
      image_pixels;
  double pixel_width_2 =
      1.0 *
      (second_outermost_pixel_mask.array() && (masked_image.array() > 0))
          .count() /
      image_pixels;
  return LeakageParameter{pixel_width_1, pixel_width_2, intensity_width_1,
                          intensity_width_2};
}
// TODO: Add the unit test for the concentration parameter
ConcentrationParameter ImageProcessor::concentration_parameter(
    const CameraGeometry &camera_geometry, const Eigen::VectorXd &masked_image,
    const HillasParameter &hillas_parameter) {
  double concentration_pixel =
      masked_image.maxCoeff() / hillas_parameter.intensity;
  auto delta_x = camera_geometry.pix_x_fov.array() - hillas_parameter.x;
  auto delta_y = camera_geometry.pix_y_fov.array() - hillas_parameter.y;
  Eigen::ArrayXd distance =
      (delta_x.array() * delta_x.array() + delta_y.array() * delta_y.array())
          .sqrt();
  auto mask_cog = distance < camera_geometry.pix_width_fov[0];
  double concentration_cog =
      masked_image.dot(mask_cog.cast<double>().matrix()) /
      hillas_parameter.intensity;

  // Rotate the axis anti-clockwise by the psi angle
  Eigen::Matrix2d rotation_matrix =
      (Eigen::Matrix2d() << cos(hillas_parameter.psi),
       sin(hillas_parameter.psi), -sin(hillas_parameter.psi),
       cos(hillas_parameter.psi))
          .finished();
  Eigen::ArrayXd delta_x_rotated = rotation_matrix.row(0)(0) * delta_x.array() +
                                   rotation_matrix.row(0)(1) * delta_y.array();
  Eigen::ArrayXd delta_y_rotated = rotation_matrix.row(1)(0) * delta_x.array() +
                                   rotation_matrix.row(1)(1) * delta_y.array();
  auto mask_core = (delta_x_rotated.array() * delta_x_rotated.array() /
                        pow(hillas_parameter.length, 2) +
                    delta_y_rotated.array() * delta_y_rotated.array() /
                        pow(hillas_parameter.width, 2)) < 1;
  double concentration_core =
      masked_image.dot(mask_core.cast<double>().matrix()) /
      hillas_parameter.intensity;
  return ConcentrationParameter{concentration_cog, concentration_core,
                                concentration_pixel};
}
MorphologyParameter
ImageProcessor::morphology_parameter(const CameraGeometry &camera_geometry,
                                     Eigen::Vector<bool, -1> &image_mask,
                                     bool only_use_largerst_island) {
  std::unordered_map<size_t, std::vector<size_t>> island_map;
  Eigen::Vector<bool, -1> pixel_in_island =
      Eigen::Vector<bool, -1>::Zero(image_mask.size());
  size_t island_id = 0;
  for (size_t i = 0; i < image_mask.size(); ++i) {
    std::queue<size_t> queue;
    if (image_mask[i] && !pixel_in_island[i]) {
      queue.push(i);
      pixel_in_island[i] = true;
      island_map[island_id].push_back(i);
      while (!queue.empty()) {
        auto pixel = queue.front();
        queue.pop();
        for (Eigen::SparseMatrix<int, Eigen::RowMajor>::InnerIterator it(
                 camera_geometry.neigh_matrix, pixel);
             it; ++it) {
          if (it.value() > 0 && !pixel_in_island[it.col()] &&
              image_mask[it.col()]) {
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
  std::vector<size_t> largest_island_pixels;
  for (const auto &[island_id, island_pixels] : island_map) {
    if (island_pixels.size() < 10) {
      n_small_islands++;
    } else if (island_pixels.size() < 30) {
      n_medium_islands++;
    } else {
      n_large_islands++;
    }
    if (island_pixels.size() > largest_island_pixels.size()) {
      largest_island_pixels = island_pixels;
    }
  }
  if (only_use_largerst_island) {
    spdlog::warn("Only using the largest island");
    image_mask = Eigen::Vector<bool, -1>::Zero(image_mask.size());
    for (auto pixel : largest_island_pixels) {
      image_mask[pixel] = true;
    }
  }

  return MorphologyParameter{n_pixels, num_island, n_small_islands,
                             n_medium_islands, n_large_islands};
}
IntensityParameter
ImageProcessor::intensity_parameter(const Eigen::VectorXd &masked_image) {
  double intensity_max = masked_image.maxCoeff();
  double intensity_mean = masked_image.sum() / masked_image.count();
  double intensity_std = 0;
  for (auto ipe : masked_image) {
    if (ipe > 0) {
      intensity_std += std::pow(ipe - intensity_mean, 2);
    }
  }
  intensity_std = std::sqrt(intensity_std / masked_image.count());

  double intensity_skewness = 0;
  double intensity_kurtosis = 0;
  if (masked_image.count() > 0) {
    double mean = intensity_mean;
    double std_dev = intensity_std;
    for (auto ipe : masked_image) {
      if (ipe > 0) {
        intensity_skewness += std::pow((ipe - mean) / std_dev, 3);
        intensity_kurtosis += std::pow((ipe - mean) / std_dev, 4);
      }
    }
    intensity_skewness /= masked_image.count();
    intensity_kurtosis /= masked_image.count();
    intensity_kurtosis -= 3; // Excess kurtosis
  }
  return IntensityParameter{intensity_max, intensity_mean, intensity_std,
                            intensity_skewness, intensity_kurtosis};
}

void ImageProcessor::dilate_image(const CameraGeometry &camera_geometry,
                                  Eigen::Vector<bool, -1> &image_mask) {
  Eigen::SparseMatrix<int, Eigen::RowMajor> dilated_matrix =
      camera_geometry.neigh_matrix;
  image_mask =
      (dilated_matrix * image_mask.cast<int>().matrix()).cast<bool>() ||
      image_mask;
}

void ImageProcessor::handle_simulation_level(ArrayEvent &event) {
  if (!event.simulation) {
    spdlog::warn("Simulation data is not available in the event");
    return;
  }
  event.simulation->triggered_tels.clear();
  for (auto &[tel_id, simulated_camera] : event.simulation->tels) {
    if (simulated_camera->true_image_sum >= 10) {
      if (poisson_noise > 0) {
        auto noise_image =
            adding_poisson_noise(simulated_camera->true_image, poisson_noise);
        if (fake_trigger(
                subarray.tels.at(tel_id).camera_description.camera_geometry,
                noise_image, fake_trigger_pe, fake_trigger_pixels)) {
          event.simulation->triggered_tels.push_back(tel_id);
        }
        Eigen::VectorXd fake_image = noise_image.array() - poisson_noise;
        if (use_random_gaussian) {
          Eigen::VectorXd random_gaussian(fake_image.size());
          std::normal_distribution<> d(1.0, random_gaussian_level);
          for (int i = 0; i < random_gaussian.size(); ++i) {
            random_gaussian(i) = d(gen);
          }
          fake_image = fake_image.array() * random_gaussian.array();
        }
        fake_image = fake_image.array().min(8000);
        simulated_camera->fake_image = std::move(fake_image);
      } else {
        simulated_camera->fake_image =
            simulated_camera->true_image.cast<double>();
      }
    }
  }

  for (const auto tel_id : event.simulation->triggered_tels) {
    auto &simulated_camera = event.simulation->tels.at(tel_id);
    auto image_mask = (*image_cleaner)(
        subarray.tels.at(tel_id).camera_description.camera_geometry,
        simulated_camera->fake_image);
    // Eigen::VectorXd leakage_masked_image =
    // image_mask.select(simulated_camera->fake_image,
    // Eigen::VectorXd::Zero(simulated_camera->fake_image.size()));
    Eigen::VectorXd masked_image = image_mask.select(
        simulated_camera->fake_image,
        Eigen::VectorXd::Zero(simulated_camera->fake_image.size()));
    if (masked_image.sum() < 50) {
      simulated_camera->image_parameters = ImageParameters();
      continue;
    }
    MorphologyParameter morphology_parameter =
        ImageProcessor::morphology_parameter(
            subarray.tels.at(tel_id).camera_description.camera_geometry,
            image_mask, only_use_largerst_island);
    // Update the masked image, it's possible that morpgology_parameter can update teh image_mask!
    masked_image = image_mask.select(
        simulated_camera->fake_image,
        Eigen::VectorXd::Zero(simulated_camera->fake_image.size()));
    HillasParameter hillas_parameter = ImageProcessor::hillas_parameter(
        subarray.tels.at(tel_id).camera_description.camera_geometry,
        masked_image);
    hillas_parameter.scale_ratio = 1.0;
    LeakageParameter leakage_parameter = ImageProcessor::leakage_parameter(
        const_cast<CameraGeometry &>(
            subarray.tels.at(tel_id).camera_description.camera_geometry),
        masked_image);
    ConcentrationParameter concentration_parameter =
        ImageProcessor::concentration_parameter(
            subarray.tels.at(tel_id).camera_description.camera_geometry,
            masked_image, hillas_parameter);
    IntensityParameter intensity_parameter =
        ImageProcessor::intensity_parameter(masked_image);

    // Don't consider second level clean for now
    TwoGaussianFitResult two_gaussian_fit;
    //TwoGaussianFitResult two_gaussian_fit = ImageProcessor::two_gaussian_fit(
    //    subarray.tels.at(tel_id).camera_description.camera_geometry,
    //    masked_image, image_mask, hillas_parameter);
    if (use_cut_radius) {
      auto pixel_mask = cut_pixel_distance(
          subarray.tels.at(tel_id).camera_description.camera_geometry,
          subarray.tels.at(tel_id).optics_description.equivalent_focal_length,
          cut_radius);
      pixel_mask = pixel_mask && image_mask;
      Eigen::VectorXd rounded_masked_image =
          masked_image.array() * pixel_mask.cast<double>().array();
      if (rounded_masked_image.sum() > 50) {
        HillasParameter rounded_hillas = ImageProcessor::hillas_parameter(
            subarray.tels.at(tel_id).camera_description.camera_geometry,
            rounded_masked_image);
        rounded_hillas.scale_ratio =
            rounded_masked_image.sum() / masked_image.sum();
        event.rounded_tel_hillas[tel_id] = rounded_hillas;
      }
    }
    simulated_camera->fake_image_mask = image_mask;
    simulated_camera->image_parameters.two_gaussian_fit = two_gaussian_fit;
    simulated_camera->image_parameters.hillas = hillas_parameter;
    simulated_camera->image_parameters.leakage = leakage_parameter;
    simulated_camera->image_parameters.concentration = concentration_parameter;
    simulated_camera->image_parameters.morphology = morphology_parameter;
    simulated_camera->image_parameters.intensity = intensity_parameter;
  }
}
Eigen::VectorXd ImageProcessor::adding_poisson_noise(Eigen::VectorXi true_image,
                                                     double poisson_noise) {
  std::poisson_distribution<int> poisson_dist(poisson_noise);
  Eigen::VectorXd noisy_image(true_image.size());
  for (int i = 0; i < true_image.size(); ++i) {
    noisy_image[i] = poisson_dist(gen) + true_image[i];
  }
  return noisy_image;
}

bool ImageProcessor::fake_trigger(const CameraGeometry &camera_geometry,
                                  const Eigen::VectorXd &image,
                                  double threshold,
                                  int min_pixels_above_threshold) {
  // Check if the image has enough pixels above the threshold
  Eigen::Vector<bool, -1> above_threshold_pixels = image.array() > threshold;
  int num_pixels_above_threshold = above_threshold_pixels.count();
  if (num_pixels_above_threshold < 5) {
    return false; // Not enough pixels above the threshold
  }
  Eigen::VectorXi pixels_above_in_group =
      camera_geometry.neigh_matrix * above_threshold_pixels.cast<int>();
  if (pixels_above_in_group.maxCoeff() < min_pixels_above_threshold) {
    return false; // Not enough pixels in the group above the threshold
  }
  return true;
}

Eigen::Vector<bool, -1>
ImageProcessor::cut_pixel_distance(const CameraGeometry &camera_geometry,
                                   double focal_length, double radius) {
  Eigen::ArrayXd pix_r_sqaure = (camera_geometry.pix_x.array().pow(2) +
                                 camera_geometry.pix_y.array().pow(2))
                                    .array();
  Eigen::Vector<bool, -1> pixel_mask =
      pix_r_sqaure <= pow(radius * focal_length * M_PI / 180, 2);
  return pixel_mask;
}
