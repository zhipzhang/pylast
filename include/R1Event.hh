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
        R1Camera(int n_pixels, int n_samples, Eigen::Matrix<double, -1, -1, Eigen::RowMajor> waveform, Eigen::VectorXi gain_selection);
        R1Camera() = default;
        ~R1Camera() = default;
        R1Camera(const R1Camera& other) = delete;
        R1Camera& operator=(const R1Camera& other) = delete;
        R1Camera(R1Camera&& other) noexcept = default;
        R1Camera& operator=(R1Camera&& other) noexcept = default;
        Eigen::Matrix<double, -1, -1, Eigen::RowMajor> waveform;
        Eigen::VectorXi gain_selection;
    private:
        int n_pixels;
        int n_samples;
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
    ~R1Event() = default;
    R1Event(const R1Event& other) = delete;
    R1Event& operator=(const R1Event& other) = delete;
    R1Event(R1Event&& other) noexcept = default;
    R1Event& operator=(R1Event&& other) noexcept = default;

};