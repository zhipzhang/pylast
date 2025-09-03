/**
 * @file RootDataLevels.cpp
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Implementation of ROOT data level structures
 * @version 0.1
 * @date 2025-03-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "RootDataLevels.hh"
#include "ArrayEvent.hh"
#include "CameraDescription.hh"
#include "ImageParameters.hh"
#include "SubarrayDescription.hh"
#include "TDirectory.h"
#include <string>
#include "spdlog/spdlog.h"
#include "tree_gemini.hh"

TTree* RootEventIndex::initialize_write(const std::string& name, const std::string& title)
{
    TTree* tree = new TTree(name.c_str(), title.c_str());
    tree->Branch("event_id", &event_id);
    tree->Branch("telescopes", &telescopes);
    return tree;
}

void RootEventIndex::initialize_read(TTree* tree)
{
    index_tree = tree;
    telescopes_ptr = &telescopes;
    tree->SetBranchAddress("event_id", &event_id);
    tree->SetBranchAddress("telescopes", &telescopes_ptr);
}

ArrayEvent RootEventHelper::get_event()
{
    ArrayEvent event;
    if(!root_event_index.has_value())
    {
        throw std::runtime_error("RootEventIndex is not initialized");
    }
    root_event_index->get_entry(current_entry);
    event.event_id = root_event_index->event_id;
    
    // Process simulation shower data (event-level, not telescope-level)
    process_event_level_data(event);
    
    // Process telescope-level data
    for (auto tel_id: root_event_index->telescopes)
    {
        process_tel_level_data(event, tel_id);
    }
    // Process DL2 reconstruction data
    process_dl2_rec_data(event);
    current_entry++;
    
    return event;
}

void RootEventHelper::process_event_level_data(ArrayEvent& event)
{
    // Process simulation shower
    if(root_simulation_shower.has_value())
    {
        auto& shower = root_simulation_shower->get_entry(current_entry);
        event.simulation = SimulatedEvent();
        event.simulation->shower = shower;
    }
    
    // Process pointing data
    if(root_pointing.has_value())
    {
        event.pointing = root_pointing->get_entry(current_entry);
    }
}

void RootEventHelper::process_tel_level_data(ArrayEvent& event, int tel_id)
{
    // Process different telescope data levels using template function
    process_tel_data_level(root_r0_camera, event.r0, tel_id);
    process_tel_data_level(root_r1_camera, event.r1, tel_id);
    process_tel_data_level(root_dl0_camera, event.dl0, tel_id);
    process_tel_data_level(root_dl1_camera, event.dl1, tel_id);
    process_tel_data_level(root_tel_monitor, event.monitor, tel_id);
    process_tel_data_level(root_dl2_camera, event.dl2, tel_id);
}

void RootEventHelper::process_dl2_rec_data(ArrayEvent& event)
{
    // Ensure DL2Event exists before processing reconstruction data
    if(!event.dl2.has_value())
    {
        event.dl2 = DL2Event();
    }
    
    // Process DL2 reconstruction data using template function
    process_dl2_rec_data_map(root_dl2_rec_geometry_map, event.dl2->geometry);
    process_dl2_rec_data_map(root_dl2_rec_energy_map, event.dl2->energy);
    process_dl2_rec_data_map(root_dl2_rec_particle_map, event.dl2->particle);
}

TelescopeDescription RootConfigHelper::get_telescope_description(int ientry)
{
    auto root_optics_description_entry = root_optics_description->get_entry(ientry);
    auto root_camera_geometry_entry = root_camera_geometry->get_entry(ientry);
    auto root_camera_readout_entry = root_camera_readout->get_entry(ientry);

    return TelescopeDescription{
        .camera_description = CameraDescription{
            .camera_name = root_camera_geometry_entry.camera_name,
            .camera_geometry = root_camera_geometry_entry,
            .camera_readout = root_camera_readout_entry
        },
        .optics_description = root_optics_description_entry,
    };

}
