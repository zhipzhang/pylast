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
#include "SimulatedShowerArray.hh"
/**
 * @brief SimtelEventSource can automatically read the simtel files and fill the DataLevel R0 and R1 and SimulatedEvent
 * 
 */
class SimtelEventSource: public EventSource 
{
public:
    SimtelEventSource() = default;
    SimtelEventSource(const string& filename, int64_t max_events = -1 , std::vector<int> subarray = {}, bool load_simulated_showers = false, int gain_selector_threshold = 4000);

    virtual ~SimtelEventSource() = default;
    const std::string print() const;
    /**
     * @brief This can be called directory or set the load_simulated_showers to true in the constructor
     *        After calling this function, the simulated showers will be loaded into the shower_array
     * 
     */
    virtual void load_all_simulated_showers() override;
    const SimulatedShowerArray& get_shower_array() {
        if(!shower_array.has_value())
        {
            shower_array = SimulatedShowerArray(std::move(simtel_file_handler->shower_array));
        }
        return *shower_array;
    }
private:
    /**
     * @brief Initialize the `SimtelFileHandler` and read util events
     * 
     */
    virtual void open_file() override;
    virtual void init_simulation_config() override;
    virtual void init_atmosphere_model() override;
    virtual void init_metaparam() override;
    virtual void init_subarray() override;

    /**
     * @brief Let the simtelfile_handler read the next event, SimtelEvent block
     * 
     * @return true 
     * @return false 
     */
    bool _load_next_event() ;
    virtual bool is_finished()  override {return simtel_file_handler->no_more_blocks;}

    /**
     * @brief Actually implementation of init_simulation_config, set the simulation configuration from the simtel file
     * 
     */
    void set_simulation_config();
    /**
     * @brief Set the telescope settings for a specific telescope
     * 
     * @param tel_id The telescope ID
     */
    void set_telescope_settings(int tel_id);
    /**
     * @brief Actually implementation of init_metaparam, set the metaparam from the simtel file
     * 
     */
    void set_metaparam();
    void read_simulated_showers(ArrayEvent& event);
    void read_true_image(ArrayEvent& event);
    void read_adc_samples(ArrayEvent& event);
    void read_monitor(ArrayEvent& event);
    void read_pointing(ArrayEvent& event);
    void apply_simtel_calibration(ArrayEvent& event);
    ArrayEvent get_event() override;
    CameraGeometry get_telescope_camera_geometry(int tel_index);
    CameraReadout get_telescope_camera_readout(int tel_index);
    OpticsDescription get_telescope_optics(int tel_index);
    std::array<double, 3> get_telescope_position(int tel_index);
    std::unique_ptr<SimtelFileHandler> simtel_file_handler;
    std::string camera_name;
    std::string optics_name;
    int gain_selector_threshold;
};

