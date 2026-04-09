/**
 * @file ImageProcessor.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief
 * @version 0.1
 * @date 2025-02-09
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include "ArrayEvent.hh"
#include "CameraDescription.hh"
#include "CameraGeometry.hh"
#include "ConfigMacros.hh"
#include "ConfigSystem.hh"
#include "Eigen/Dense"
#include "ImageCleaner.hh"
#include "ImageParameters.hh"
#include "SimulatedCamera.hh"
#include "SubarrayDescription.hh"
#include <memory>

class ImageProcessor : public config::Configurable {
public:
  CONFIG_PARAM_CONSTRUCTORS(ImageProcessor, const SubarrayDescription &,
                            subarray);
  ~ImageProcessor() = default;

  static Eigen::Vector<bool, -1>
  tailcuts_clean(const CameraGeometry &camera_geometry,
                 const Eigen::VectorXd &image, double picture_thresh,
                 double boundary_thresh, bool keep_isolated_pixels = false,
                 int min_number_picture_neighbors = 0);

  void registerParams() override;
  void setUp() override;

  void operator()(ArrayEvent &event);
  Eigen::Vector<bool, -1> image_clean(const CameraGeometry &camera_geometry,
                                      const Eigen::VectorXd &image);
  static HillasParameter hillas_parameter(const CameraGeometry &camera_geometry,
                                          const Eigen::VectorXd &masked_image);
  
  static LeakageParameter
  leakage_parameter(const CameraGeometry &camera_geometry,
                    const Eigen::VectorXd &masked_image);
  static TwoGaussianFitResult
  two_gaussian_fit(const CameraGeometry &camera_geometry,
                   const Eigen::VectorXd &image,
                   const Eigen::Vector<bool, -1> &image_mask,
                   const HillasParameter &hillas_parameter);
  static ConcentrationParameter
  concentration_parameter(const CameraGeometry &camera_geometry,
                          const Eigen::VectorXd &masked_image,
                          const HillasParameter &hillas_parameter);
  static MorphologyParameter
  morphology_parameter(const CameraGeometry &camera_geometry,
                       Eigen::Vector<bool, -1> &image_mask,
                       bool only_use_largerst_island = false);
  static IntensityParameter
  intensity_parameter(const Eigen::VectorXd &masked_image);
  static void dilate_image(const CameraGeometry &camera_geometry,
                           Eigen::Vector<bool, -1> &image_mask);
  static Eigen::Vector<bool, -1>
  cut_pixel_distance(const CameraGeometry &camera_geometry, double focal_length,
                     double cut_radius);

private:
  const SubarrayDescription &subarray;
  std::string image_cleaner_type;
  std::unique_ptr<ImageCleaner> image_cleaner;
  double poisson_noise = 0.0;
  int fake_trigger_pixels = 0;
  double fake_trigger_pe = 0;
  double cut_radius = 0.0;
  bool use_cut_radius = false;
  bool use_random_gaussian = false;
  double random_gaussian_level = 0;
  bool only_use_largerst_island = false;
  bool use_gaussian_fit = false;
  void handle_simulation_level(ArrayEvent &event);
  bool fake_trigger(const CameraGeometry &camera_geometry,
                    const Eigen::VectorXd &image, double threshold,
                    int min_pixels_above_threshold = 4);
  Eigen::VectorXd adding_poisson_noise(Eigen::VectorXi true_image,
                                       double poisson_noise);

  
};
