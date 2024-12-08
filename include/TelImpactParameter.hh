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
    TelImpactParameter() = default;
    ~TelImpactParameter() = default;
    TelImpactParameter(double impact_parameter, double impact_parameter_error):
        impact_parameter(impact_parameter), impact_parameter_error(impact_parameter_error) {}
    double impact_parameter;
    double impact_parameter_error;
};