/**
 * @file SimulatedShower.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  class to describe a simulated shower
 * @version 0.1
 * @date 2024-12-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once
#include <string>
#include "spdlog/fmt/fmt.h"
 class SimulatedShower {
public:
    /** @brief Default constructor */
    SimulatedShower() = default;
    /** @brief Default destructor */
    ~SimulatedShower() = default;

    /** @brief Simulated primary particle energy in TeV */
    double energy;
    /** @brief Simulated shower altitude angle in degrees */
    double alt;
    /** @brief Simulated shower azimuth angle in degrees */
    double az;
    /** @brief Simulated shower core x position in meters */
    double core_x;
    /** @brief Simulated shower core y position in meters */
    double core_y;
    /** @brief Height of first interaction in meters */
    double h_first_int;
    /** @brief Simulated shower maximum (Xmax) in g/cm^2 */
    double x_max;
    /** @brief Maximum height of shower in m */
    double h_max;
    /** @brief Atmospheric depth where primary particle was injected in g/cm^2 */
    double starting_grammage;
    /** @brief Primary particle ID number */
    int shower_primary_id;
    const std::string print() const {
        return fmt::format("SimulatedShower(\n"
                         "    shower_primary_id={}\n"
                         "    energy={} TeV,\n"
                         "    alt={} deg,\n"
                         "    az={} deg,\n" 
                         "    core_x={} m,\n"
                         "    core_y={} m,\n"
                         "    h_first_int={} m,\n"
                         "    x_max={} g/cm^2,\n"
                         "    starting_grammage={} g/cm^2,\n"
                         ")",
                         shower_primary_id, energy, alt, az,
                         core_x, core_y, h_first_int, x_max,
                         starting_grammage);
    }
 };