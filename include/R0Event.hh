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

#include <unordered_map>
#include <Eigen/Dense>

class R0Camera
{
    public:
        R0Camera() = default;
        ~R0Camera() = default;
        R0Camera(const R0Camera& other) = delete;
        R0Camera& operator=(const R0Camera& other) = delete;
        R0Camera(R0Camera&& other) noexcept = default;
        R0Camera& operator=(R0Camera&& other) noexcept = default;
        std::array<Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>, 2> waveform;
        std::array<Eigen::Vector<uint32_t, -1>, 2> waveform_sum;
        void set_waveform(int n_pixels, int n_samples, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>&& high_gain, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>&& low_gain);
        void set_waveform_sum(int n_pixels, Eigen::Vector<uint32_t, -1>&& high_gain_sum, Eigen::Vector<uint32_t, -1>&& low_gain_sum);
    private:
        int n_pixels;
        int n_samples;
        inline void initialize_waveform(int n_pixels, int n_samples);
        inline void initialize_waveform_sum(int n_pixels);
};
class R0Event{
public:
    R0Event() = default;
    ~R0Event() = default;
    R0Event(const R0Event& other) = delete;
    R0Event& operator=(const R0Event& other) = delete;
    R0Event(R0Event&& other) noexcept = default;
    R0Event& operator=(R0Event&& other) noexcept = default;
    std::unordered_map<int, R0Camera> tels;
};