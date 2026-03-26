/**
 * @file C0Event.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Class to store the Calibration Raw Data waveform for two gains.
 * @version 0.1
 * @date 2026-03-25
 * 
 * @copyright Copyright (c) 2026
 * 
 */

 #pragma once
 #include "BaseTelContainer.hh"
 #include <Eigen/Dense>
 class C0Camera
 {
    public:
    // num_pixel, num_samples
    int n_pixels;
    int n_samples;
    Eigen::Matrix<float, -1, -1, Eigen::RowMajor> low_gain_waveform;
    Eigen::Matrix<float, -1, -1, Eigen::RowMajor> high_gain_waveform;
 };
 class C0Event: public BaseTelContainer<C0Camera>
 {
    public:
    C0Event() = default;
 };