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
    TelMonitor(int n_channels, int n_pixels, double* pedestal_per_sample, double* dc_to_pe, int max_pixels);
    virtual ~TelMonitor() = default;
    TelMonitor(const TelMonitor&) = delete;
    TelMonitor& operator=(const TelMonitor&) = delete;
    TelMonitor(TelMonitor&&) = default;
    TelMonitor& operator=(TelMonitor&&) = default;
    std::array<Eigen::VectorXd, 2> pedestal_per_sample;
    std::array<Eigen::VectorXd, 2> dc_to_pe;
    int n_channels;
    int n_pixels;
 };