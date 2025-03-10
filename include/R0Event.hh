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
class R0Camera
{
    public:
        R0Camera(int n_pixels, int n_samples, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> high_gain_waveform, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> low_gain_waveform, Eigen::Vector<uint32_t, -1> high_gain_waveform_sum, Eigen::Vector<uint32_t, -1> low_gain_waveform_sum);
        R0Camera(int n_pixels, int n_samples, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> high_gain_waveform, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> low_gain_waveform)
        {
            this->n_pixels = n_pixels;
            this->n_samples = n_samples;
            waveform[0] = std::move(high_gain_waveform);
            waveform[1] = std::move(low_gain_waveform);
        }
        ~R0Camera() = default;
        R0Camera(const R0Camera& other) = delete;
        R0Camera& operator=(const R0Camera& other) = delete;
        R0Camera(R0Camera&& other) noexcept = default;
        R0Camera& operator=(R0Camera&& other) noexcept = default;
        std::array<Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>, 2> waveform;
        std::optional<std::array<Eigen::Vector<uint32_t, -1>, 2>> waveform_sum;
    private:
        int n_pixels;
        int n_samples;
};
class R0Event: public BaseTelContainer<R0Camera>{
public:
    R0Event() = default;
    ~R0Event() = default;
    R0Event(const R0Event& other) = delete;
    R0Event& operator=(const R0Event& other) = delete;
    R0Event(R0Event&& other) noexcept = default;
    R0Event& operator=(R0Event&& other) noexcept = default;
    void add_tel(int tel_id, int n_pixels, int n_samples, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> high_gain_waveform, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> low_gain_waveform, uint32_t* high_gain_waveform_sum, uint32_t* low_gain_waveform_sum);
};