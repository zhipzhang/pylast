/**
 * @file RootWriter.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  Root writer for root files
 * @version 0.1
 * @date 2025-02-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "DataWriter.hh"
#include "DataWriterFactory.hh"
#include "TFile.h"
#include "TTree.h"
#include "ROOT/RVec.hxx"
#include "RootDataLevels.hh"

class RootWriter: public FileWriter
{
    public:
        RootWriter(EventSource& source, const std::string& filename);
        virtual ~RootWriter()
        {
            /*
            for(auto& [name, tree] : trees)
            {
                spdlog::info("Writing tree: {}", name);
                tree->Write();
                delete tree;
            }
            */
        }
        //void operator()(const ArrayEvent& event) override;
        void open(bool overwrite = false) override;
        void close() override;
        //void write_metaparam() override;
        void write_atmosphere_model() override;
        void write_subarray() override;
        void write_simulation_config() override;
        // Methods for writing specific parts of an ArrayEvent
        void write_simulation_shower(const ArrayEvent& event) override;
        void write_simulated_camera(const ArrayEvent& event) override {};
        void write_r0(const ArrayEvent& event) override;
        void write_r1(const ArrayEvent& event) override;
        void write_dl0(const ArrayEvent& event) override;
        void write_dl1(const ArrayEvent& event, bool write_image = false) override;
        void write_dl2(const ArrayEvent& event) override;
        void write_monitor(const ArrayEvent& event) override;
        void write_pointing(const ArrayEvent& event) override;
        
        void write_statistics(const Statistics& statistics) override;
        // Write event with components based on configuration
        void write_event(const ArrayEvent& event) override;
        void unique_write_method(const ArrayEvent& event) override
        {
            write_event_index(event);
        }
        //void write_simulation_config() override;
    private:
        std::unique_ptr<TFile> file;
        RootArrayEvent array_event;
        void write_event_index(const ArrayEvent& event);
        
        template<typename T>
        void initialize_data_level(const std::string& level_name, std::optional<T>& data_level);
        // Helper methods for initializing branches
        void initialize_simulation_config_branches(TTree& tree, SimulationConfiguration& config);
        void initialize_simulation_shower_branches(TTree& tree, SimulatedShower& shower);
        // Helper method to create or get a directory
        TDirectory* get_or_create_directory(const std::string& path);
        TTree* create_tree(const std::string& tree_name, const std::string& title, TDirectory* dir);
        TTree* get_tree(const std::string& tree_name);
        std::unordered_map<std::string, TTree*> trees;
        std::unordered_map<std::string, bool> build_index;
        std::unordered_map<std::string, TDirectory*> directories;
        int event_id;
        int tel_id;
};

