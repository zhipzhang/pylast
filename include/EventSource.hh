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
#include "SimulationConfiguration.hh"
#include <vector>
#include "AtmosphereModel.hh"
#include "Metaparam.hh"
#include "SubarrayDescription.hh"
#include "ArrayEvent.hh"
#include <optional>
#include "Statistics.hh"
#include "SimulatedShowerArray.hh"
using std::string;
class EventSource
{
public:
    class Iterator{
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = ArrayEvent;
            using pointer = ArrayEvent*;
            using reference = ArrayEvent&;
            Iterator(EventSource* source, int position):source_(source), position_(position){
                if(position == 0 && source_){
                    *source_->current_event = std::move(source_->get_event());
                }
            }
            ArrayEvent& operator*() const {return *source_->current_event;}

            bool operator==(const Iterator& other) const{
                if (source_ && source_->is_finished()) {
                    return true;
                }
                return source_ == other.source_ && position_ == other.position_;
            }
            bool operator!=(const Iterator& other) const{
                return !(*this==other);
            }
            /**
             * @brief If we have the next event and position_ not exceed max_events, we will read the next event
             * 
             * @return Iterator& 
             */
            Iterator& operator++()
            {
                ++position_;
                source_->current_event_index = position_;
                if(source_ && !source_->is_finished() && (source_->max_events == -1 || position_ < source_->max_events)){
                    *source_->current_event = source_->get_event();
                }
                return *this;
            }
        private:
            EventSource* source_;
            int position_;
    };
    EventSource() = default;
    EventSource(const string& filename) : input_filename(filename) {}
    EventSource(const string& filename, int64_t max_events , std::vector<int>& subarray , bool load_simulated_showers = false):input_filename(filename), max_events(max_events), allowed_tels(subarray), load_simulated_showers(load_simulated_showers) {}
    virtual ~EventSource() = default;
    virtual void load_all_simulated_showers() = 0;

    /**
     * @brief The input filename
     */
    string input_filename;

    bool load_simulated_showers;
    /**
     * @brief Whether it can support random access
     */
    bool is_stream = false;
    int64_t max_events;
    /**
     * @brief Can be used to select the telescope for later analysis
     */
    std::vector<int> allowed_tels;
    /**
     * @brief The current event index [0, max_events)
     */
    int current_event_index = 0;
    
    /**
     * @brief The simulation configuration, mainly the input card of corsika
     */
    std::optional<SimulationConfiguration> simulation_config;

    /**
     * @brief The subarray description, includes the camera/optics of each telescope
     */
    std::optional<SubarrayDescription> subarray;

    std::optional<Statistics> statistics;
    std::optional<TableAtmosphereModel> atmosphere_model;
    std::optional<Metaparam> metaparam;

    /**
     * @brief load_simulated_showers is used to load the simulated showers into the shower_array
     */
    std::optional<SimulatedShowerArray> shower_array;   
    /**
     * @brief The current read event
     */
    std::optional<ArrayEvent> current_event;
    Iterator begin(){
        if(!current_event){
            current_event.emplace();
        }
        auto it =  Iterator(this, current_event_index);
        return it;
    }
    Iterator end(){return Iterator(this, max_events);}
protected:
    /**
     * @brief Check if we still have events to read
     * 
     * @return true 
     * @return false 
     */
    virtual bool is_finished() = 0;
    /**
     * @brief Read/Init the simulation configuration, mainly the input card of corsika
     * 
     */
    virtual void init_simulation_config() = 0;
    /**
     * @brief Read the next event
     * 
     * @return ArrayEvent 
     */
    virtual ArrayEvent get_event() = 0;
    /**
     * @brief Read the atmosphere model
     * 
     */
    virtual void init_atmosphere_model() = 0;
    /**
     * @brief Read the metaparam
     * 
     */
    virtual void init_metaparam() = 0;
    /**
     * @brief Read the subarray description, includes the camera/optics of each telescope
     * 
     */
    virtual void init_subarray() = 0;

    bool is_subarray_selected(int tel_id) const;
    virtual void open_file() = 0;

    
    void initialize() {
    try{
        open_file();
        init_metaparam();
        init_simulation_config();
        init_atmosphere_model();
        init_subarray();
        if(load_simulated_showers){
            load_all_simulated_showers();
        }
    } 
    catch(const std::exception& e) {
        throw std::runtime_error("Error initializing EventSource: " + std::string(e.what()) + " Open file: " + input_filename);
    }
}

};



