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
#include "CalibEvent.hh"
#include "DL0Event.hh"
#include "DL1Event.hh"
#include "DL2Event.hh"
#include "EventMonitor.hh"
#include "ImageParameters.hh"
#include "Pointing.hh"
#include "R0Event.hh"
#include "R1Event.hh"
#include "SimulatedEvent.hh"
#include <optional>
#include <unordered_map>
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
  std::optional<CalibEvent> c1;
  std::optional<EventMonitor> monitor;
  std::optional<DL0Event> dl0;
  std::optional<DL1Event> dl1;
  std::optional<Pointing> pointing;
  std::optional<DL2Event> dl2;
  std::unordered_map<int, HillasParameter> rounded_tel_hillas;
  int event_id;
  int run_id;
  double mjd = -1; // Only used in real-data.
};