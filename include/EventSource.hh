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
            Iterator& operator++()
            {
                ++position_;
                source_->current_event_index = position_;
                if(source_ && !source_->is_finished() && (source_->max_events == -1 || position_ < source_->max_events)){
                    *source_->current_event = std::move(source_->get_event());
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
    string input_filename;

    /** @brief Whether the file is a stream(e.g. eos file system) */
    bool is_stream = false;

    std::optional<SimulationConfiguration> simulation_config;
    std::optional<SubarrayDescription> subarray;

    int64_t max_events;
    std::vector<int> allowed_tels;
    std::optional<TableAtmosphereModel> atmosphere_model;
    std::optional<Metaparam> metaparam;
    std::optional<SimulatedShowerArray> shower_array;   
    std::optional<ArrayEvent> current_event;
    int current_event_index = 0;
    Iterator begin(){
        if(!current_event){
            current_event = ArrayEvent();
        }
        auto it =  Iterator(this, current_event_index);
        return it;
    }
    Iterator end(){return Iterator(this, max_events);}
    virtual void load_all_simulated_showers() = 0;
protected:
    virtual bool is_finished() const = 0;
    virtual void init_simulation_config() = 0;
    virtual ArrayEvent get_event() = 0;
    virtual void init_atmosphere_model() = 0;
    virtual void init_metaparam() = 0;
    virtual void init_subarray() = 0;
    bool is_subarray_selected(int tel_id) const;
    bool load_simulated_showers;
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
    } catch(const std::exception& e) {
        throw std::runtime_error("Error initializing EventSource: " + std::string(e.what()));
        }
    }

};



