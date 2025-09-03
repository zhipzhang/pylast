/**
 * @file ReconstructedGeometry.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  store the information of the reconstructed geometry
 * @version 0.1
 * @date 2025-03-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once
#include <vector>
class ReconstructedGeometry
{
    public:
        bool is_valid;
        double alt;
        double alt_uncertainty;
        double az;
        double az_uncertainty;
        double direction_error;
        double core_x;
        double core_y;
        double core_pos_error;
        double tilted_core_x;
        double tilted_core_y;
        double tilted_core_uncertainty_x;
        double tilted_core_uncertainty_y;
        double hmax;
        std::vector<int> telescopes;
        
};

class ReconstructedEnergy
{
    public:
        bool energy_valid = false;
        double estimate_energy = 0;
        std::vector<int> telescopes;
};

class ReconstructedParticle
{
    public:
        double hadroness = 0;
        bool is_valid = false;
        std::vector<int> telescopes;
};