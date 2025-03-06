/**
 * @file ArrayEvent.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief core class to describe an array event
 * @version 0.1
 * @date 2024-12-07
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
 class ArrayEvent {
public:
    ArrayEvent() = default;
    ~ArrayEvent() = default;
    ArrayEvent(const ArrayEvent& other) = delete;
    ArrayEvent& operator=(const ArrayEvent& other) = delete;

    ArrayEvent(ArrayEvent&& other) noexcept = default;
    ArrayEvent& operator=(ArrayEvent&& other) noexcept = default;
    std::optional<SimulatedEvent> simulation;
    std::optional<R0Event> r0;
    std::optional<R1Event> r1;
    std::optional<EventMonitor> monitor;
    std::optional<DL0Event> dl0;
    std::optional<DL1Event> dl1;
    std::optional<Pointing> pointing;
    std::optional<DL2Event> dl2;
    //std::optional<R1Event> r1_event;
};