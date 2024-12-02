/**
 * @file SimtelEventSource.hh
 * @author Zach Peng
 * @brief Class to read simtel files
 * @version 0.1
 * @date 2024-11-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include "EventSource.hh"
#include <unordered_map>
#include <memory>
#include "SimtelFileHandler.hh"
class SimtelEventSource: public EventSource 
{
public:
    SimtelEventSource() = default;
    SimtelEventSource(const string& filename, int64_t max_events = -1 , std::vector<int> subarray = {});
    virtual ~SimtelEventSource() = default;
    void print_history();
    const std::string print() const;
private:
    virtual void init_simulation_config() override;
    //virtual void init_subarray() override;
    virtual void init_atmosphere_model() override;
    void set_simulation_config();
    std::unique_ptr<SimtelFileHandler> simtel_file_handler;
};

