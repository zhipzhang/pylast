/**
 * @file  LACT1Calibrator.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Tempoary class for converting peak to p.e. for LACT-1 telesscope
 * @version 0.1
 * @date 2026-01-26
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once
#include "ArrayEvent.hh"
#include "Configurable.hh"

/**
 * @brief Select the gain channel by threshold
 *
 * @param waveform  2-channel waveform (assume the first channel is the high
 * gain channel, sometimes the second channel is 0)
 * @param threshold threshold for the low gain channel
 * @return Eigen::VectorXi
 */
Eigen::VectorXi tmp_select_gain_channel_by_threshold(
    Eigen::Matrix<double, -1, -1, Eigen::RowMajor> &peak,
    const double threshold);

class LACT1Calibrator : public Configurable {

public:
  DECLARE_CONFIGURABLE_CONSTRUCTORS(LACT1Calibrator);
  void configure(const json &config) override { return; };
  json default_config() const override { return json(); };
  void operator()(ArrayEvent &event);
  void load_calibration_file(const std::string &calibration_file);
  Eigen::Map<Eigen::VectorXd> get_low_gain_calibration_factor(const double event_mjd);
  Eigen::Map<Eigen::VectorXd> get_high_gain_calibration_factor(const double event_mjd);

private:
  static constexpr double high_gain_mv_to_pe = 2.5;
  static constexpr double low_gain_mv_to_pe = 10.2;
  static constexpr double threshold =
      36; // Threshold to use high gain or low_gain
  bool have_calibration_file = false;
  Eigen::VectorXd calibration_time;
  // Row-major: each row holds the per-pixel factors for one calibration time,
  // so `.row(i).data()` is contiguous and safe to wrap in Eigen::Map.
  Eigen::Matrix<double, -1, -1, Eigen::RowMajor> calibration_low_gain_area;
  Eigen::Matrix<double, -1, -1, Eigen::RowMajor> calibration_high_gain_area;

  Eigen::Index find_nearest_calibration_index(double event_mjd) const;

  double convert_linux_time_to_mjd(double linux_time);
};