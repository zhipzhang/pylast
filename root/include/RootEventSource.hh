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

 #include "ArrayEvent.hh"
 #include "EventSource.hh"
 #include "RootDataLevels.hh"
 #include <memory>
 #include "TFile.h"
 #include "RootHistogram.hh"
 class RootEventSource : public EventSource
 {
    public:
        RootEventSource(const std::string& filename, int64_t max_events, std::vector<int> subarray, bool load_subarray_from_env = false);
        virtual ~RootEventSource()
        {
            file->Close();
        }
        virtual void open_file() override;
        virtual void init_metaparam() override;
        virtual void init_atmosphere_model() override;
        virtual void init_subarray() override;
        virtual void init_simulation_config() override;
        virtual void load_all_simulated_showers() override;
        virtual ArrayEvent get_event() override;
        virtual ArrayEvent get_event(int index) override ;
        virtual bool is_finished()  override;


    private:
        void initialize_array_event();
        template<typename T>
        void initialize_data_level(const std::string& level_name, std::optional<T>& data_level);
        template<typename T>
        void initialize_dl2_trees(const std::string& subdir, std::unordered_map<std::string, std::optional<T>>& tree_map);
        template<typename T>
        void initialize_dir(const std::string& subdir, const std::string& tree_name, std::optional<T>& structure);

        void initialize_event_index();
        RootEventHelper event_helper;
        RootConfigHelper config_helper;
        void initialize_statistics();
        bool load_subarray_from_env;  // In case we can read subarray from environment variable
        std::unique_ptr<TFile> file;
 };