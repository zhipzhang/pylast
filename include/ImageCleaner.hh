/**
 * @file ImageCleaner.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2025-02-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include "Configurable.hh"
#include "CameraGeometry.hh"
#include "Eigen/Dense"
using json = nlohmann::json;
class ImageCleaner
{
public:
    ImageCleaner() = default;
    virtual ~ImageCleaner() = default;
    virtual Eigen::Vector<bool, -1> operator()(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image) const = 0;
};
class TailcutsCleaner: public ImageCleaner, public Configurable
{
public:
    DECLARE_CONFIGURABLE_CONSTRUCTORS(TailcutsCleaner);
    static json get_default_config() ;
    json default_config() const override;
    void configure(const json& config) override;
    Eigen::Vector<bool, -1> operator()(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image) const override;
    static Eigen::Vector<bool, -1> tailcuts_clean(const CameraGeometry& camera_geometry, const Eigen::VectorXd& image, double picture_thresh, double boundary_thresh, bool keep_isolated_pixels = false, int min_number_picture_neighbors = 0);
    double get_picture_thresh() const {return picture_thresh;}
    double get_boundary_thresh() const {return boundary_thresh;}
    bool get_keep_isolated_pixels() const {return keep_isolated_pixels;}
    int get_min_number_picture_neighbors() const {return min_number_picture_neighbors;}
private:
    double picture_thresh;
    double boundary_thresh;
    bool keep_isolated_pixels;
    int min_number_picture_neighbors;
};
