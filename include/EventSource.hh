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
                    event_ = std::move(source_->get_event());
                }
            }
            const ArrayEvent& operator*() const {return event_;}
            const ArrayEvent* operator->() const {return &event_;};

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
                if(source_ && !source_->is_finished() && (source_->max_events == -1 || position_ < source_->max_events)){
                    event_ = std::move(source_->get_event());
                }
                return *this;
            }
        private:
            EventSource* source_;
            int position_;
            ArrayEvent event_;
    };
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
    std::optional<SimulatedShowerArray> shower_array;   
    Iterator begin(){ return Iterator(this, 0);}
    Iterator end(){return Iterator(this, max_events);}
    virtual void load_all_simulated_showers() = 0;
protected:
    virtual bool is_finished() const = 0;
    virtual void init_simulation_config() = 0;
    //virtual void init_subarray() = 0;
    virtual ArrayEvent get_event() = 0;
    virtual void init_atmosphere_model() = 0;
    virtual void init_metaparam() = 0;
    virtual void init_subarray() = 0;
    bool is_subarray_selected(int tel_id) const;
};

