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
#include "CameraGeometry.hh"
#include "SubarrayDescription.hh"
#include "Eigen/Dense"
#include "Configurable.hh"
#include <memory>
#include "ImageCleaner.hh"
class ImageProcessor: public Configurable
{
public:
    DECLARE_CONFIGURABLE_DEFINITIONS(const SubarrayDescription&, subarray, ImageProcessor);
    ~ImageProcessor() = default;
    static Eigen::Vector<bool, -1> tailcuts_clean(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image, double picture_thresh, double boundary_thresh, bool keep_isolated_pixels = false, int min_number_picture_neighbors = 0);
    void configure(const json& config) override;
    static json get_default_config();
    json default_config() const override {return get_default_config();}
private:
    const SubarrayDescription& subarray;
    std::string image_cleaner_type;
    std::unique_ptr<ImageCleaner> image_cleaner;

};


