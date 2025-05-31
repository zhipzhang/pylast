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

    /**
     * @brief Image Processor parameters
     * 
     */
    ImageParameters image_parameters;
    
    /**
     * @brief Get the image data from the DL0 DataLevel
     * 
     */
    Eigen::VectorXf image;
    /**
     * @brief Get the peak time data from the DL0 DataLevel
     * 
     */
    Eigen::VectorXf peak_time;
    /**
     * @brief Get the pixel mask after tail_cuts clean
     * 
     */
    Eigen::Vector<bool, -1> mask;
};
class DL1Event: public BaseTelContainer<DL1Camera>
{
    public:
    DL1Event() = default;
};