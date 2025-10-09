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
#include "CameraDescription.hh"
#include "CameraGeometry.hh"
#include "SimulatedCamera.hh"
#include "SubarrayDescription.hh"
#include "Eigen/Dense"
#include "ConfigSystem.hh"
#include "ConfigMacros.hh"
#include <memory>
#include "ImageCleaner.hh"
#include "ArrayEvent.hh"
#include "ImageParameters.hh"

class ImageProcessor: public config::Configurable
{
public:
    CONFIG_PARAM_CONSTRUCTORS(ImageProcessor, const SubarrayDescription&, subarray);
    ~ImageProcessor() = default;
    
    static Eigen::Vector<bool, -1> tailcuts_clean(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image, double picture_thresh, double boundary_thresh, bool keep_isolated_pixels = false, int min_number_picture_neighbors = 0);
    
    void registerParams() override;
    void setUp() override;
    
    void operator()(ArrayEvent& event);
    static HillasParameter hillas_parameter(const CameraGeometry& camera_geometry, const Eigen::VectorXd& masked_image);
    static LeakageParameter leakage_parameter(CameraGeometry& camera_geometry, const Eigen::VectorXd& masked_image);
    static ConcentrationParameter concentration_parameter(const CameraGeometry& camera_geometry, const Eigen::VectorXd& masked_image, const HillasParameter& hillas_parameter);
    static MorphologyParameter morphology_parameter(const CameraGeometry& camera_geometry, const Eigen::Vector<bool, -1>& image_mask);
    static IntensityParameter intensity_parameter(const Eigen::VectorXd& masked_image);
    static void dilate_image(const CameraGeometry& camera_geometry, Eigen::Vector<bool, -1>& image_mask);
    static Eigen::Vector<bool, -1> cut_pixel_distance(const CameraGeometry& camera_geometry, double focal_length,  double cut_radius);
    
    
private:
    const SubarrayDescription& subarray;
    std::string image_cleaner_type;
    std::unique_ptr<ImageCleaner> image_cleaner;
    double poisson_noise = 0.0;
    double cut_radius = 0.0;
    bool use_cut_radius = false;
    void handle_simulation_level(ArrayEvent& event);
    bool fake_trigger(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image, double threshold, int min_pixels_above_threshold = 4);
    Eigen::VectorXd adding_poisson_noise(Eigen::VectorXi true_image, double poisson_noise);
};


