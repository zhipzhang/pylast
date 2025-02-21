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
    ~WaveformCalibrator() = default;
    WaveformCalibrator(const WaveformCalibrator&) = delete;
    WaveformCalibrator& operator=(const WaveformCalibrator&) = delete;
    WaveformCalibrator(WaveformCalibrator&&) = default;
    WaveformCalibrator& operator=(WaveformCalibrator&&) = default;
    // Following matrix is <channel, pixel>

 };
  class TelMonitor
 {
   public:
    TelMonitor() = default;
    TelMonitor(int n_channels, int n_pixels, Eigen::Matrix<double, -1, -1, Eigen::RowMajor> pedestal_per_sample, Eigen::Matrix<double, -1, -1, Eigen::RowMajor> dc_to_pe);
    virtual ~TelMonitor() = default;
    TelMonitor(const TelMonitor&) = delete;
    TelMonitor& operator=(const TelMonitor&) = delete;
    TelMonitor(TelMonitor&&) = default;
    TelMonitor& operator=(TelMonitor&&) = default;
    Eigen::Matrix<double, -1, -1, Eigen::RowMajor> pedestal_per_sample;
    Eigen::Matrix<double, -1, -1, Eigen::RowMajor> dc_to_pe;
    int n_channels;
    int n_pixels;
 };