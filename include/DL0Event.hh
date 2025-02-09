/**
 * @file DL0Event.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2025-01-31
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once
#include "BaseTelContainer.hh"
#include <Eigen/Dense>
class DL0Camera
{
    public:
    DL0Camera() = default;
    ~DL0Camera() = default;
    DL0Camera(Eigen::VectorXd image, Eigen::VectorXd peak_time):
        image(std::move(image)),
        peak_time(std::move(peak_time))
    {}
    DL0Camera(const DL0Camera& other) = delete;
    DL0Camera& operator=(const DL0Camera& other) = delete;
    DL0Camera(DL0Camera&& other) noexcept = default;
    DL0Camera& operator=(DL0Camera&& other) noexcept = default;

    Eigen::VectorXd image;
    Eigen::VectorXd peak_time;
};

class DL0Event: public BaseTelContainer<DL0Camera>
{
    public:
    DL0Event() = default;
    ~DL0Event() = default;

    DL0Event(const DL0Event& other) = delete;
    DL0Event& operator=(const DL0Event& other) = delete;
    DL0Event(DL0Event&& other) noexcept = default;
    DL0Event& operator=(DL0Event&& other) noexcept = default;
};