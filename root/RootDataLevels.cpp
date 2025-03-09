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
#include "TDirectory.h"
#include <string>
#include "spdlog/spdlog.h"
//------------------------------------------------------------------------------
// RootEventIndex implementation
//------------------------------------------------------------------------------

TTree* RootEventIndex::initialize(const std::string& name, const std::string& title)
{
    TTree* tree = new TTree(name.c_str(), title.c_str());
    tree->Branch("event_id", &event_id);
    tree->Branch("telescopes", &telescopes);
    return tree;
}

void RootEventIndex::initialize(TTree* tree)
{
    tree->SetBranchAddress("event_id", &event_id);
    tree->SetBranchAddress("telescopes", &telescopes);
}

//------------------------------------------------------------------------------
// RootR0Event implementation
//------------------------------------------------------------------------------

TTree* RootR0Event::initialize()
{
    auto tree = new TTree("tels", "R0 data for all telescopes");
    tree->Branch("event_id", &event_id);
    tree->Branch("tel_id", &tel_id);
    tree->Branch("n_pixels", &n_pixels);
    tree->Branch("n_samples", &n_samples);
    tree->Branch("low_gain_waveform", &low_gain_waveform);
    tree->Branch("high_gain_waveform", &high_gain_waveform);
    return tree;
}

void RootR0Event::initialize(TTree* tree)
{
    tree->SetBranchAddress("event_id", &event_id);
    tree->SetBranchAddress("tel_id", &tel_id);
    tree->SetBranchAddress("n_pixels", &n_pixels);
    tree->SetBranchAddress("n_samples", &n_samples);
    tree->SetBranchAddress("low_gain_waveform", &low_gain_waveform);
    tree->SetBranchAddress("high_gain_waveform", &high_gain_waveform);
}

//------------------------------------------------------------------------------
// RootR1Event implementation
//------------------------------------------------------------------------------

TTree* RootR1Event::initialize()
{
    TTree* tree = new TTree("tels", "R1 data for all telescopes");
    tree->Branch("event_id", &event_id);
    tree->Branch("tel_id", &tel_id);
    tree->Branch("n_pixels", &n_pixels);
    tree->Branch("waveform", &waveform);
    tree->Branch("gain_selection", &gain_selection);
    return tree;
}

void RootR1Event::initialize(TTree* tree)
{
    tree->SetBranchAddress("event_id", &event_id);
    tree->SetBranchAddress("tel_id", &tel_id);
    tree->SetBranchAddress("n_pixels", &n_pixels);
    tree->SetBranchAddress("waveform", &waveform);
    tree->SetBranchAddress("gain_selection", &gain_selection);
}

//------------------------------------------------------------------------------
// RootDL0Event implementation
//------------------------------------------------------------------------------

TTree* RootDL0Event::initialize()
{
    TTree* tree = new TTree("tels", "DL0 data for all telescopes");
    tree->Branch("event_id", &event_id);
    tree->Branch("tel_id", &tel_id);
    tree->Branch("n_pixels", &n_pixels);
    tree->Branch("image", &image);
    tree->Branch("peak_time", &peak_time);
    return tree;
}

void RootDL0Event::initialize(TTree* tree)
{
    tree->SetBranchAddress("event_id", &event_id);
    tree->SetBranchAddress("tel_id", &tel_id);
    tree->SetBranchAddress("n_pixels", &n_pixels);
    tree->SetBranchAddress("image", &image);
    tree->SetBranchAddress("peak_time", &peak_time);
}

//------------------------------------------------------------------------------
// RootDL1Event implementation
//------------------------------------------------------------------------------

TTree* RootDL1Event::initialize()
{
    TTree* tree = new TTree("tels", "DL1 data for all telescopes");
    tree->Branch("event_id", &event_id);
    tree->Branch("tel_id", &tel_id);

    // Image parameters
    tree->Branch("hillas_length", &params.hillas.length);
    tree->Branch("hillas_width", &params.hillas.width);
    tree->Branch("hillas_x", &params.hillas.x);
    tree->Branch("hillas_y", &params.hillas.y);
    tree->Branch("hillas_phi", &params.hillas.phi);
    tree->Branch("hillas_psi", &params.hillas.psi);
    tree->Branch("hillas_r", &params.hillas.r);
    tree->Branch("hillas_skewness", &params.hillas.skewness);
    tree->Branch("hillas_kurtosis", &params.hillas.kurtosis);
    tree->Branch("hillas_intensity", &params.hillas.intensity);
    // Leakage parameters
    tree->Branch("leakage_pixels_width_1", &params.leakage.pixels_width_1);
    tree->Branch("leakage_pixels_width_2", &params.leakage.pixels_width_2);
    tree->Branch("leakage_intensity_width_1", &params.leakage.intensity_width_1);
    tree->Branch("leakage_intensity_width_2", &params.leakage.intensity_width_2);
    
    // Concentration parameters
    tree->Branch("concentration_cog", &params.concentration.concentration_cog);
    tree->Branch("concentration_core", &params.concentration.concentration_core);
    tree->Branch("concentration_pixel", &params.concentration.concentration_pixel);
    
    // Morphology parameters
    tree->Branch("morphology_num_pixels", &params.morphology.n_pixels);
    tree->Branch("morphology_num_islands", &params.morphology.n_islands);
    tree->Branch("morphology_num_small_islands", &params.morphology.n_small_islands);
    tree->Branch("morphology_num_medium_islands", &params.morphology.n_medium_islands);
    tree->Branch("morphology_num_large_islands", &params.morphology.n_large_islands);
    
    
    
    return tree;
}
TTree* RootDL1Event::initialize(bool have_image)
{
    if(!have_image)
    {
        return initialize();
    }
    else
    {
        auto tree = initialize();
        tree->Branch("image", &image);
        tree->Branch("peak_time", &peak_time);
        tree->Branch("mask", &mask);
        tree->Branch("n_pixels", &n_pixels);
        return tree;
    }
}

void RootDL1Event::initialize(TTree* tree)
{
    // Basic telescope data
    if(tree->GetBranch("image"))
    {
        spdlog::debug("DL1 have stored image parts");
        tree->SetBranchAddress("event_id", &event_id);
        tree->SetBranchAddress("tel_id", &tel_id);
        tree->SetBranchAddress("n_pixels", &n_pixels);
        tree->SetBranchAddress("image", &image);
        tree->SetBranchAddress("peak_time", &peak_time);
        tree->SetBranchAddress("mask", &mask);
    }
    // Now handle the image parameters
    // Hillas parameters
    tree->SetBranchAddress("hillas_length", &params.hillas.length);
    tree->SetBranchAddress("hillas_width", &params.hillas.width);
    tree->SetBranchAddress("hillas_psi", &params.hillas.psi);
    tree->SetBranchAddress("hillas_x", &params.hillas.x);
    tree->SetBranchAddress("hillas_y", &params.hillas.y);
    tree->SetBranchAddress("hillas_skewness", &params.hillas.skewness);
    tree->SetBranchAddress("hillas_kurtosis", &params.hillas.kurtosis);
    tree->SetBranchAddress("hillas_intensity", &params.hillas.intensity);
    tree->SetBranchAddress("hillas_r", &params.hillas.r);
    tree->SetBranchAddress("hillas_phi", &params.hillas.phi);
    
    // Leakage parameters
    tree->SetBranchAddress("leakage_pixels_width_1", &params.leakage.pixels_width_1);
    tree->SetBranchAddress("leakage_pixels_width_2", &params.leakage.pixels_width_2);
    tree->SetBranchAddress("leakage_intensity_width_1", &params.leakage.intensity_width_1);
    tree->SetBranchAddress("leakage_intensity_width_2", &params.leakage.intensity_width_2);
    
    // Concentration parameters
    tree->SetBranchAddress("concentration_cog", &params.concentration.concentration_cog);
    tree->SetBranchAddress("concentration_core", &params.concentration.concentration_core);
    tree->SetBranchAddress("concentration_pixel", &params.concentration.concentration_pixel);
    
    // Morphology parameters
    tree->SetBranchAddress("morphology_num_pixels", &params.morphology.n_pixels);
    tree->SetBranchAddress("morphology_num_islands", &params.morphology.n_islands);
    tree->SetBranchAddress("morphology_num_small_islands", &params.morphology.n_small_islands);
    tree->SetBranchAddress("morphology_num_medium_islands", &params.morphology.n_medium_islands);
    tree->SetBranchAddress("morphology_num_large_islands", &params.morphology.n_large_islands);

}

//------------------------------------------------------------------------------
// RootPointing implementation
//------------------------------------------------------------------------------

TTree* RootPointing::initialize()
{
    TTree* tree = new TTree("pointing", "Pointing data for all telescopes");
    tree->Branch("event_id", &event_id);
    tree->Branch("tel_id", &tel_id);
    tree->Branch("tel_az", &tel_az);
    tree->Branch("tel_alt", &tel_alt);
    tree->Branch("array_az", &array_az);
    tree->Branch("array_alt", &array_alt);
    return tree;
}

void RootPointing::initialize(TTree* tree)
{
    tree->SetBranchAddress("event_id", &event_id);
    tree->SetBranchAddress("tel_id", &tel_id);
    tree->SetBranchAddress("tel_az", &tel_az);
    tree->SetBranchAddress("tel_alt", &tel_alt);
    tree->SetBranchAddress("array_az", &array_az);
    tree->SetBranchAddress("array_alt", &array_alt);
}

//------------------------------------------------------------------------------
// RootMonitor implementation
//------------------------------------------------------------------------------

TTree* RootMonitor::initialize()
{
    TTree* tree = new TTree("monitor", "Monitor data for all telescopes");
    tree->Branch("event_id", &event_id);
    tree->Branch("tel_id", &tel_id);
    tree->Branch("n_channels", &n_channels);
    tree->Branch("n_pixels", &n_pixels);
    tree->Branch("dc_to_pe", &dc_to_pe);
    tree->Branch("pedestals", &pedestals);
    return tree;
}

void RootMonitor::initialize(TTree* tree)
{
    tree->SetBranchAddress("event_id", &event_id);
    tree->SetBranchAddress("tel_id", &tel_id);
    tree->SetBranchAddress("n_channels", &n_channels);
    tree->SetBranchAddress("n_pixels", &n_pixels);
    tree->SetBranchAddress("dc_to_pe", &dc_to_pe);
    tree->SetBranchAddress("pedestals", &pedestals);
}
//------------------------------------------------------------------------------
// RootSimulationShower implementation
//------------------------------------------------------------------------------

TTree* RootSimulationShower::initialize()
{
    TTree* tree = new TTree("shower", "Simulation shower data");
    tree->Branch("event_id", &event_id);
    tree->Branch("energy", &shower.energy);
    tree->Branch("alt", &shower.alt);
    tree->Branch("az", &shower.az);
    tree->Branch("core_x", &shower.core_x);
    tree->Branch("core_y", &shower.core_y);
    tree->Branch("h_first_int", &shower.h_first_int);
    tree->Branch("x_max", &shower.x_max);
    tree->Branch("starting_grammage", &shower.starting_grammage);
    tree->Branch("shower_primary_id", &shower.shower_primary_id);
    return tree;
}

void RootSimulationShower::initialize(TTree* tree)
{
    tree->SetBranchAddress("event_id", &event_id);
    tree->SetBranchAddress("shower", &shower);
    tree->SetBranchAddress("energy", &shower.energy);
    tree->SetBranchAddress("alt", &shower.alt);
    tree->SetBranchAddress("az", &shower.az);
    tree->SetBranchAddress("core_x", &shower.core_x);
    tree->SetBranchAddress("core_y", &shower.core_y);
    tree->SetBranchAddress("h_first_int", &shower.h_first_int);
    tree->SetBranchAddress("x_max", &shower.x_max);
}

//------------------------------------------------------------------------------
// RootDL2GeometryEvent implementation
//------------------------------------------------------------------------------

TTree* RootDL2Geometry::initialize()
{
    TTree* tree = new TTree(reconstructor_name.c_str(), "DL2 geometry reconstruction");
    tree->Branch("event_id", &event_id);
    tree->Branch("is_valid", &geometry.is_valid);
    tree->Branch("alt", &geometry.alt);
    tree->Branch("alt_uncertainty", &geometry.alt_uncertainty);
    tree->Branch("az", &geometry.az);
    tree->Branch("az_uncertainty", &geometry.az_uncertainty);
    tree->Branch("direction_error", &geometry.direction_error);
    tree->Branch("core_x", &geometry.core_x);
    tree->Branch("core_y", &geometry.core_y);
    tree->Branch("core_pos_error", &geometry.core_pos_error);
    tree->Branch("tilted_core_x", &geometry.tilted_core_x);
    tree->Branch("tilted_core_y", &geometry.tilted_core_y);
    tree->Branch("tilted_core_uncertainty_x", &geometry.tilted_core_uncertainty_x);
    tree->Branch("tilted_core_uncertainty_y", &geometry.tilted_core_uncertainty_y);
    tree->Branch("hmax", &geometry.hmax);
    tree->Branch("telescopes", &geometry.telescopes);
    return tree;
}

void RootDL2Geometry::initialize(TTree* tree)
{
    tree->SetBranchAddress("event_id", &event_id);
    tree->SetBranchAddress("is_valid", &geometry.is_valid);
    tree->SetBranchAddress("alt", &geometry.alt);
    tree->SetBranchAddress("alt_uncertainty", &geometry.alt_uncertainty);
    tree->SetBranchAddress("az", &geometry.az);
    tree->SetBranchAddress("az_uncertainty", &geometry.az_uncertainty);
    tree->SetBranchAddress("direction_error", &geometry.direction_error);
    tree->SetBranchAddress("core_x", &geometry.core_x);
    tree->SetBranchAddress("core_y", &geometry.core_y);
    tree->SetBranchAddress("core_pos_error", &geometry.core_pos_error);
    tree->SetBranchAddress("tilted_core_x", &geometry.tilted_core_x);
    tree->SetBranchAddress("tilted_core_y", &geometry.tilted_core_y);
    tree->SetBranchAddress("tilted_core_uncertainty_x", &geometry.tilted_core_uncertainty_x);
    tree->SetBranchAddress("tilted_core_uncertainty_y", &geometry.tilted_core_uncertainty_y);
    tree->SetBranchAddress("hmax", &geometry.hmax);
    tree->SetBranchAddress("telescopes", &geometry.telescopes);
}

TTree* RootDL2Event::initialize()
{
    TTree* tree = new TTree("tels", "DL2 data for all telescopes");
    tree->Branch("event_id", &event_id);
    tree->Branch("tel_id", &tel_id);
    tree->Branch("reconstructor_name", &reconstructor_name);
    tree->Branch("distance", &distance);
    tree->Branch("distance_error", &distance_error);
    return tree;
}

void RootDL2Event::initialize(TTree* tree)
{
    tree->SetBranchAddress("event_id", &event_id);
    tree->SetBranchAddress("tel_id", &tel_id);
    tree->SetBranchAddress("reconstructor_name", &reconstructor_name);
    tree->SetBranchAddress("distance", &distance);
    tree->SetBranchAddress("distance_error", &distance_error);
}