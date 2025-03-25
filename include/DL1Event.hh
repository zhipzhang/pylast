/**
 * @file DL1Event.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2025-02-10
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include "ImageParameters.hh"
#include <Eigen/Dense>
#include "BaseTelContainer.hh"
class DL1Camera
{
    public:
    DL1Camera() = default;
    ~DL1Camera() = default;
    DL1Camera(const DL1Camera& other) = delete;
    DL1Camera& operator=(const DL1Camera& other) = delete;
    DL1Camera(DL1Camera&& other) noexcept = default;
    DL1Camera& operator=(DL1Camera&& other) noexcept = default;

    ImageParameters image_parameters;
    Eigen::VectorXf image;
    Eigen::VectorXf peak_time;
    Eigen::Vector<bool, -1> mask;
};
class DL1Event: public BaseTelContainer<DL1Camera>
{
    public:
    DL1Event() = default;
    ~DL1Event() = default;
    DL1Event(const DL1Event& other) = delete;
    DL1Event& operator=(const DL1Event& other) = delete;
    DL1Event(DL1Event&& other) noexcept = default;
    DL1Event& operator=(DL1Event&& other) noexcept = default;
};