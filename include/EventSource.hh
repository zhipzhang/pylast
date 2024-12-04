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
#include "SimulationConfiguration.hh"
#include <vector>
#include "AtmosphereModel.hh"
#include "Metaparam.hh"
#include "SubarrayDescription.hh"
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
    SubarrayDescription subarray;

    int64_t max_events;
    std::vector<int> allowed_tels;
    TableAtmosphereModel atmosphere_model;
    Metaparam metaparam;
protected:

    virtual void init_simulation_config() = 0;
    //virtual void init_subarray() = 0;
    virtual void init_atmosphere_model() = 0;
    virtual void init_metaparam() = 0;
    virtual void init_subarray() = 0;
    bool is_subarray_selected(int tel_id) const;
};

