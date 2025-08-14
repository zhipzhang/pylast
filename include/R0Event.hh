/**
 * @file R0Event.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2024-12-27
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include <cstdint>
#include <unordered_map>
#include <Eigen/Dense>
#include "BaseTelContainer.hh"
#include <optional>

using WaveformMatrix = Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>;
using WaveformSumVector = Eigen::Vector<uint32_t, -1>;
class R0Camera
{
    public:
        int n_pixels;
        int n_samples;
        std::array<WaveformMatrix, 2> waveform;
        std::array<WaveformSumVector, 2> waveform_sum;
};
class R0Event: public BaseTelContainer<R0Camera>{
public:
    R0Event() = default;
};