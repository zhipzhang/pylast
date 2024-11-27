/**
 * @file SubarrayDescription.hh
 * @author Zach Peng 
 * @brief  class to describe the subarray of LACT telescopes
 * @version 0.1
 * @date 2024-11-27
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once
#include "Basebind.hh"
#include <unordered_map>

using std::unordered_map;
class TelescopeDescription;
class SubarrayDescription: public Basebind
{
    public:
        SubarrayDescription() = default;
        ~SubarrayDescription() = default;
        /** @brief Map of telescope id to telescope description */
        unordered_map<int, TelescopeDescription> tel_descriptions;
        /** @brief Map of telescope id to telescope position */
        unordered_map<int, std::array<float, 3>> tel_positions;
};

