/**
 * @file TelImpactParameter.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  class to describe the impact parameter of a telescope
 * @version 0.1
 * @date 2024-12-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

struct TelImpactParameter {
public:
    double distance;
    double distance_error = 0.0;
};