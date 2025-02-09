/**
 * @file Utils.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Some utilities
 * @version 0.1
 * @date 2024-12-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */
 #pragma once

#include <cmath>
#include <Eigen/Dense>
namespace Utils {
/**
 * @brief Calculate the distance between a point and a line in 3D space
 * @param point The 3D point coordinates as array<double,3>
 * @param line_point A point on the line as array<double,3>
 * @param line_direction Direction vector of the line as array<double,3> (does not need to be normalized)
 * @return double The shortest distance from the point to the line
 */
template<typename T>
double point_line_distance(const std::array<T,3>& point,
                         const std::array<T,3>& line_point, 
                         const std::array<T,3>& line_direction) {
    // Convert arrays to Eigen vectors
    Eigen::Vector3d p(point[0], point[1], point[2]);
    Eigen::Vector3d lp(line_point[0], line_point[1], line_point[2]);
    Eigen::Vector3d ld(line_direction[0], line_direction[1], line_direction[2]);

    // Calculate vector from line point to the point
    Eigen::Vector3d vec = p - lp;

    // Calculate cross product and get distance using Eigen operations
    return (vec.cross(ld)).norm() / ld.norm();
}

 /**
  * @brief Select the gain channel by threshold
  * 
  * @param waveform  2-channel waveform (assume the first channel is the high gain channel, sometimes the second channel is 0)
  * @param threshold threshold for the low gain channel
  * @return Eigen::VectorXi
  */
template<typename T>
Eigen::VectorXi select_gain_channel_by_threshold(const std::array<Eigen::Matrix<T, -1, -1, Eigen::RowMajor>, 2>& waveform, const double threshold)
{
    // Matrix is (n_pixels, n_samples)
    // Vector returned is (n_pixels)
    if(waveform[1].isZero())
    {
        return Eigen::VectorXi::Zero(waveform[0].rows());
    }
    else 
    {
        // If the high gain channel exceeds the threshold, select the low gain channel
        Eigen::VectorXi gain_selector = Eigen::VectorXi::Zero(waveform[0].rows());
        for(int i = 0; i < waveform[0].rows(); i++)
        {
            if((waveform[0].row(i).array() > threshold).any())
            {
                gain_selector(i) = 1;
            }
        }
        return gain_selector;
    }
}
}