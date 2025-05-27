/**
 * @file ArrayEvent.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief core class to describe an array event
 * @version 0.2
 * @date 2024-12-07
 * 
 * @changelog
 * - v0.2: Follow the rule of zero, clear up the code.
 *
 * @copyright Copyright (c) 2024
 * 
 */
 #pragma once
#include "SimulatedEvent.hh"
#include <optional>
#include "R0Event.hh"
#include "R1Event.hh"
#include "EventMonitor.hh"
#include "DL0Event.hh"
#include "DL1Event.hh"
#include "DL2Event.hh"
#include "Pointing.hh"
/**
 * @brief Main class to describe an array event
 * 
 */
class ArrayEvent {
public:
    ArrayEvent() = default;
    std::optional<SimulatedEvent> simulation;
    std::optional<R0Event> r0;
    std::optional<R1Event> r1;
    std::optional<EventMonitor> monitor;
    std::optional<DL0Event> dl0;
    std::optional<DL1Event> dl1;
    std::optional<Pointing> pointing;
    std::optional<DL2Event> dl2;
    int event_id;
    int run_id;
};