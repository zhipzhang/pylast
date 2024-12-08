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
}