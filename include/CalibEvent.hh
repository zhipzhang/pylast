/**
 * @file CalibEvent.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Tempoary for LACT-1 Calibration
 * @version 0.1
 * @date 2026-01-25
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "BaseTelContainer.hh"
#include "Eigen/Dense"

class CalibCamera {
public:
  Eigen::Matrix<double, -1, -1, Eigen::RowMajor> base; // Channal * Gain
  Eigen::Matrix<double, -1, -1, Eigen::RowMajor> peak;
  Eigen::Matrix<double, -1, -1, Eigen::RowMajor> area;
  Eigen::Matrix<double, -1, -1, Eigen::RowMajor> count_to_pe;
  Eigen::Matrix<double, -1, -1, Eigen::RowMajor> pe;
};
class CalibEvent : public BaseTelContainer<CalibCamera> {
public:
  CalibEvent() = default;
};