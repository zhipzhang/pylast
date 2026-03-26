/**
 * @file C1Event.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief including the base, peak, area of the signal
 * @version 0.1
 * @date 2026-03-26
 * 
 * @copyright Copyright (c) 2026
 * 
 */

 #pragma once
#include "BaseTelContainer.hh"
#include <Eigen/Dense>
class C1Camera
{
    public:
    int n_pixels;
    Eigen::VectorXf low_gain_base;
    Eigen::VectorXf high_gain_base;
    Eigen::VectorXf low_gain_peak;
    Eigen::VectorXf high_gain_peak;
    Eigen::VectorXf low_gain_area;
    Eigen::VectorXf high_gain_area;
    Eigen::VectorXi low_gain_peak_time;
    Eigen::VectorXi high_gain_peak_time;

};
class C1Event: public BaseTelContainer<C1Camera>
{
    public:
    C1Event() = default;
};