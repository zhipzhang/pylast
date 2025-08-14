/**
 * @file TelMonitor.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Telescope Monitor for each events (Flat-field, pedestal, pixel_status)
 * @version 0.1
 * @date 2025-01-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #pragma once
 #include <optional>
 #include "Eigen/Dense"

 class WaveformCalibrator
 {
   public:
    WaveformCalibrator() = default;

 };
  class TelMonitor
 {
   public:
    int n_channels;
    int n_pixels;
    Eigen::Matrix<double, -1, -1, Eigen::RowMajor> pedestal_per_sample;
    Eigen::Matrix<double, -1, -1, Eigen::RowMajor> dc_to_pe;
 };