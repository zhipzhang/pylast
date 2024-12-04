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

#include "CameraGeometry.hh"
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
    const std::string print() const;
private:
    virtual void init_simulation_config() override;
    virtual void init_atmosphere_model() override;
    virtual void init_metaparam() override;
    virtual void init_subarray() override;
    void set_simulation_config();
    void set_telescope_settings(int tel_id);
    CameraGeometry get_telescope_camera_geometry(int tel_index);
    CameraReadout get_telescope_camera_readout(int tel_index);
    OpticsDescription get_telescope_optics(int tel_index);
    std::array<double, 3> get_telescope_position(int tel_index);
    void set_metaparam();
    std::unique_ptr<SimtelFileHandler> simtel_file_handler;
    std::string camera_name;
    std::string optics_name;
};

