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
#include "Eigen/src/Core/Matrix.h"
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
    /** @brief Pixel x positions in the fov frame [rad] */
    VectorXd pix_x_fov;
    /** @brief Pixel y positions in the fov frame [rad] */
    VectorXd pix_y_fov;
    /** @brief Pixel areas [m^2] */
    VectorXd pix_area;
    /** @brief Pixel types 0 for circle, 1 for hexagon, 2 for square */
    Eigen::VectorXi pix_type;
    /** @brief Camera rotation [degree] */
    double cam_rotation;
    /** @brief Neighbor matrix  row i is the neighbor of pixel i */
    Eigen::SparseMatrix<int, Eigen::RowMajor> neigh_matrix;
    /** @brief Map from width to border pixel mask */
    std::unordered_map<int, Eigen::Vector<bool, -1>> border_pixel_mask; 
    /** @brief Pixel width [m] */
    Eigen::VectorXd pix_width;
    /** @brief Pixel width in the fov frame [rad] */
    Eigen::VectorXd pix_width_fov; 
    CameraGeometry() = default;
    /**
     * @brief Construct a new Camera Geometry object
     * 
     * @param camera_name name of the camera
     * @param num_pixels number of pixels in the camera
     * @param pix_x pix_x positions [m]
     * @param pix_y pix_y positions [m]
     * @param pix_area pixel areas [m^2]
     * @param pix_type pixel types
     * @param cam_rotation camera rotation [degree]
     */
    CameraGeometry(std::string camera_name, int num_pixels, double* pix_x, double* pix_y, double* pix_area, int* pix_type, double cam_rotation);
    CameraGeometry(std::string camera_name, int num_pixels, Eigen::VectorXd pix_x, Eigen::VectorXd pix_y, Eigen::VectorXd pix_area, Eigen::VectorXi pix_type, double cam_rotation);


    const string print() const;
    /**
     * @brief Get the border pixel within a certain width 
     *        
     * @param width 
     * @return Eigen::Vector<bool, -1> 
     */
    Eigen::Vector<bool, -1> get_border_pixel_mask(int width);
    Eigen::VectorXd get_pix_x_fov() const
    {
        if(pix_x_fov.size() == 0)
        {
            return pix_x;
        }
        return pix_x_fov;
    }
    Eigen::VectorXd get_pix_y_fov() const
    {
        if(pix_y_fov.size() == 0)
        {
            return pix_y;
        }
        return pix_y_fov;
    }
    void compute_neighbor_matrix(bool diagonal = false);
};
