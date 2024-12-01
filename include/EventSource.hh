/**
 * @file BaseRawSource.hh
 * @author Zach Peng
 * @brief Base class to read raw event files
 * @version 0.1
 * @date 2024-11-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#pragma once

#include <initializer_list>
#include <string>
#include "nanobind/nanobind.h"
#include "SimulationConfiguration.hh"
#include <vector>
#include "nanobind/stl/vector.h"
#include "AtmosphereModel.hh"
namespace nb = nanobind;
using std::string;
class EventSource
{
public:
    EventSource() = default;
    EventSource(const string& filename) : input_filename(filename) {}
    EventSource(const string& filename, int64_t max_events , std::vector<int>& subarray ):input_filename(filename), max_events(max_events), allowed_tels(subarray) {}
    virtual ~EventSource() = default;
    string input_filename;

    /** @brief Whether the file is a stream(e.g. eos file system) */
    bool is_stream = false;

    SimulationConfiguration simulation_config;
    static void bind(nb::module_& m) {
        nb::class_<EventSource>(m, "EventSource")
            .def_ro("input_filename", &EventSource::input_filename)
            .def_ro("is_stream", &EventSource::is_stream)
            .def_ro("max_events", &EventSource::max_events)
            .def_ro("allowed_tels", &EventSource::allowed_tels)
            .def_ro("simulation_config", &EventSource::simulation_config)
            .def_ro("atmosphere_model", &EventSource::atmosphere_model);
    }
    int64_t max_events;
    std::vector<int> allowed_tels;
    TableAtmosphereModel atmosphere_model;
protected:
    virtual void open_file(const string& filename) = 0;

    virtual void init_simulation_config() = 0;
    //virtual void init_subarray() = 0;
    virtual void init_atmosphere_model() = 0;
    bool is_subarray_selected(int tel_id) const;
};

