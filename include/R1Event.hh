/**
 * @file R1Event.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2024-12-27
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <unordered_map>
#include "BaseTelContainer.hh"
#include <Eigen/Dense>
class R1Camera
{
    public:
        int n_pixels;
        int n_samples;
        Eigen::Matrix<double, -1, -1, Eigen::RowMajor> waveform;
        Eigen::VectorXi gain_selection;
};
/**
 * @brief R1Event doing three steps:
 * 1. Select the gain channel by threshold
 * 2. Pedestal subtraction
 * 3. Convert the ADC counts to pe for each sample
 * 
 */
class R1Event: public BaseTelContainer<R1Camera>{
public:
    R1Event() = default;
};