/**
 * @file CameraGeometry.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Geometry of the camera
 * @version 0.1
 * @date 2024-11-28
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include "Eigen/Dense"
#include "Eigen/Sparse"
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::string;

class CameraGeometry
{
public:
    /** @brief Name of the camera */
    std::string camera_name;
    /** @brief Number of pixels in the camera */
    int num_pixels;
    /** @brief Pixel IDs */
    Eigen::VectorXi pix_id;
    /** @brief Pixel x positions [m] */
    VectorXd pix_x;
    /** @brief Pixel y positions [m] */
    VectorXd pix_y;
    /** @brief Pixel areas [m^2] */
    VectorXd pix_area;
    /** @brief Pixel types */
    Eigen::VectorXi pix_type;
    /** @brief Camera rotation [degree] */
    double cam_rotation;
    /** @brief Neighbor matrix */
    //SparseMatrix<double> neigh_matrix;
    

    CameraGeometry() = default;
    CameraGeometry(std::string camera_name, int num_pixels, double* pix_x, double* pix_y, double* pix_area, int* pix_type, double cam_rotation);

    CameraGeometry(CameraGeometry&& other) noexcept = default;
    CameraGeometry(const CameraGeometry& other) = default;
    CameraGeometry& operator=(CameraGeometry&& other) noexcept = default;
    CameraGeometry& operator=(const CameraGeometry& other) = default;
    ~CameraGeometry() = default;
    const string print() const;
};
