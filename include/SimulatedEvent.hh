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
#include <BaseTelContainer.hh>

using telescope_id = int;
class SimulatedEvent: public BaseTelContainer<SimulatedCamera>{
public:
    SimulatedEvent() = default;
    ~SimulatedEvent() = default;
    SimulatedEvent(SimulatedEvent&&) = default;
    SimulatedEvent& operator=(SimulatedEvent&&) noexcept = default;
    SimulatedShower shower;
    void add_simulated_image(telescope_id tel_id, int n_pixels, int* pe_count, double impact_parameter) 
    {
        BaseTelContainer::add_tel(tel_id, n_pixels, pe_count, impact_parameter);
    }

};