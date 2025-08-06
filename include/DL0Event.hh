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
    Eigen::VectorXd image;
    Eigen::VectorXd peak_time;
};

class DL0Event: public BaseTelContainer<DL0Camera>
{
    public:
    DL0Event() = default;
};