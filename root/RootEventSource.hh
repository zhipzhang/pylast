/**
 * @file RootEventSource.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  Event source for root files
 * @version 0.1
 * @date 2025-02-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #pragma once

 #include "EventSource.hh"
 #include "RootDataLevels.hh"
 #include <memory>
 #include "TFile.h"
 class RootEventSource : public EventSource
 {
    public:
        RootEventSource(const std::string& filename, int64_t max_events, std::vector<int> subarray, bool load_subarray_from_env = false);
        virtual ~RootEventSource() = default;
        virtual void open_file() override;
        virtual void init_metaparam() override;
        virtual void init_atmosphere_model() override;
        virtual void init_subarray() override;
        virtual void init_simulation_config() override;
        virtual void load_all_simulated_showers() override;
        virtual ArrayEvent get_event() override {return ArrayEvent();}
        virtual bool is_finished() const override {return false;}


    private:
        bool load_subarray_from_env;  // In case we can read subarray from environment variable
        int64_t max_events;
        std::unique_ptr<TFile> file;
 };