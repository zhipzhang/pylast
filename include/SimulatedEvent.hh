/**
 * @file SimulatedEvent.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  class to describe a simulated event, events include simulated showers and simulated camera images
 * @version 0.1
 * @date 2024-12-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include "SimulatedShower.hh"
#include "SimulatedCamera.hh"
#include <unordered_map>

using telescope_id = int;
class SimulatedEvent {
public:
    SimulatedEvent() = default;
    ~SimulatedEvent() = default;
    SimulatedEvent(const SimulatedEvent&) = default;
    SimulatedEvent(SimulatedEvent&&) = default;
    SimulatedEvent& operator=(const SimulatedEvent&) noexcept = default;
    SimulatedEvent& operator=(SimulatedEvent&&) noexcept = default;
    SimulatedShower shower;
    std::unordered_map<telescope_id, SimulatedCamera> cameras;


};