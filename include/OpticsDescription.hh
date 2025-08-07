/**
 * @file OpticsDescription.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Description of the optics
 * @version 0.1
 * @date 2024-12-02
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include <string>
using std::string;

class OpticsDescription
{
public:
    /** @brief Name of the optics */
    string optics_name;
    /** @brief Number of mirrors in the telescope */
    int num_mirrors;
    /** @brief Total reflective area of the mirrors [m^2] */
    double mirror_area;
    /** @brief Equivalent focal length of the telescope [m] */
    double equivalent_focal_length;
    /** @brief Effective focal length of the telescope [m] */
    double effective_focal_length;
    const string print() const;
};