/**
 * @file RootDataLevels.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  Structure for storing data levels in ROOT files
 * @version 0.1
 * @date 2025-03-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once
#include "ROOT/RVec.hxx"
#include "TTree.h"
#include "ImageParameters.hh"
#include "ReconstructedGeometry.hh"
#include "SimulatedShower.hh"
#include "TelImpactParameter.hh"
#include <optional>
using namespace ROOT;

/**
 * @brief Base class for all ROOT data level structures
 * 
 * Provides common interface for initializing ROOT trees and branches
 */
class RootDataLevels
{
   public:
       RootDataLevels() = default;
       virtual ~RootDataLevels() = default;
       
       /**
        * @brief Initialize a new tree with branches for this data level
        * @return Pointer to the initialized tree
        */
       virtual TTree* initialize() = 0;
       
       /**
        * @brief Initialize branches on an existing tree, this method is for reading from existing files
        * @param tree Pointer to the tree to initialize
        */
       virtual void initialize(TTree* tree) = 0;
};

/**
 * @brief Index structure for telescope data in an event
 */
class RootEventIndex 
{
   public:
       RootEventIndex() = default;
       virtual ~RootEventIndex() = default;
       
       // Data members
       int event_id;
       RVecI telescopes;
       
        TTree* initialize(const std::string& name, const std::string& title);
        void initialize(TTree* tree);
};

/**
 * @brief Structure for R0 (raw waveform) data
 */
class RootR0Event : public RootDataLevels
{
   public:
       RootR0Event() = default;
       virtual ~RootR0Event() = default;
       
       // Individual Telescope Data
       int event_id;
       int tel_id;
       int n_pixels;
       int n_samples;
       RVec<uint16_t> low_gain_waveform;
       RVec<uint16_t> high_gain_waveform;
       
       virtual TTree* initialize() override;
       virtual void initialize(TTree* tree) override;
};

/**
 * @brief Structure for R1 (calibrated waveform) data
 */
class RootR1Event : public RootDataLevels
{
   public:
       RootR1Event() = default;
       virtual ~RootR1Event() = default;
       
       // Individual Telescope Data
       int event_id;
       int tel_id;
       int n_pixels;
       int n_samples;
       RVecD waveform;
       RVecI gain_selection;
       
       virtual TTree* initialize() override;
       virtual void initialize(TTree* tree) override;
};

/**
 * @brief Structure for DL0 (integrated charge) data
 */
class RootDL0Event : public RootDataLevels
{
   public:
       RootDL0Event() = default;
       virtual ~RootDL0Event() = default;
       
       // Individual Telescope Data
       int event_id;
       int tel_id;
       int n_pixels;
       RVecD image;
       RVecD peak_time;
       
       virtual TTree* initialize() override;
       virtual void initialize(TTree* tree) override;
};

/**
 * @brief Structure for DL1 (parameterized images) data
 */
class RootDL1Event : public RootDataLevels
{
   public:
       RootDL1Event() = default;
       virtual ~RootDL1Event() = default;
       
       // Individual Telescope Data
       int event_id;
       int tel_id;
       int n_pixels;
       RVecD image;
       RVecD peak_time;
       RVec<bool> mask;
       
       // Image parameters (complete structure)
       ImageParameters params;
       
       virtual TTree* initialize() override;
       TTree* initialize(bool have_image);
       virtual void initialize(TTree* tree) override;
};

/**
 * @brief Structure for DL2 (reconstructed shower) geometry data, directory /events/dl2/geometry
 */
class RootDL2Geometry : public RootDataLevels
{
   public:
       RootDL2Geometry(const std::string& reconstructor_name) : reconstructor_name(reconstructor_name) {}
       virtual ~RootDL2Geometry() = default;
       
       // Event identification
       int event_id;
       std::string reconstructor_name; // use name to initialize the TTree
       
       // Complete reconstructed geometry structure
       ReconstructedGeometry geometry;
       virtual TTree* initialize() override;
       virtual void initialize(TTree* tree) override;
};

/**
 * @brief Structure for DL2 telescope impact parameters, events/dl2/
 */
class RootDL2Event : public RootDataLevels
{
   public:
       RootDL2Event() = default;
       virtual ~RootDL2Event() = default;
       // Event and telescope identification
       int event_id;
       int tel_id;
       std::vector<std::string> reconstructor_name;
       
       // Impact parameters structure
       std::vector<double> distance;
       std::vector<double> distance_error;
       void clear()
       {
           reconstructor_name.clear();
           distance.clear();
           distance_error.clear();
       }
       virtual TTree* initialize() override;
       virtual void initialize(TTree* tree) override;
};

/**
 * @brief Structure for monitor data
 */
class RootMonitorEvent : public RootDataLevels
{
   public:
       RootMonitorEvent() = default;
       virtual ~RootMonitorEvent() = default;
       
       // Event and telescope identification
       int event_id;
       int tel_id;
       int n_channels;
       int n_pixels;
       
       // Monitor data
       RVecD dc_to_pe;
       std::vector<RVecD> pedestals;  // Per channel
       
       virtual TTree* initialize() override;
       virtual void initialize(TTree* tree) override;
};

/**
 * @brief Structure for pointing data
 */
class RootPointing : public RootDataLevels
{
   public:
       RootPointing() = default;
       virtual ~RootPointing() = default;
       
       // Event identification
       int event_id;
       
       // Array pointing
       double array_az;
       double array_alt;
       
       // Telescope pointings
       RVecI tel_id;
       RVecD tel_az;
       RVecD tel_alt;
       
       virtual TTree* initialize() override;
       virtual void initialize(TTree* tree) override;
};

/**
 * @brief Structure for monitoring data
 * 
 */
class RootMonitor : public RootDataLevels
{
   public:
       RootMonitor() = default;
       virtual ~RootMonitor() = default;
       
       // Event identification
       int event_id;
       int tel_id;
       int n_channels;
       int n_pixels;
       
       // Monitor data
       RVecD dc_to_pe;
       RVecD pedestals;  // Per channel
       
       virtual TTree* initialize() override;
       virtual void initialize(TTree* tree) override;
};

/**
 * @brief Structure for simulation shower data
 */
class RootSimulationShower : public RootDataLevels
{
   public:
       RootSimulationShower() = default;
       virtual ~RootSimulationShower() = default;
       
       // Event identification
       int event_id;
       
       // Complete shower structure
       SimulatedShower shower;
       
       virtual TTree* initialize() override;
       virtual void initialize(TTree* tree) override;
};

class RootArrayEvent
{
    public:
        RootArrayEvent() = default;
        virtual ~RootArrayEvent() = default;
        void initialize_writer();
        std::optional<RootSimulationShower> simulation;
        std::optional<RootEventIndex> r0_index;
        std::optional<RootR0Event> r0;
        std::optional<RootEventIndex> r1_index;
        std::optional<RootR1Event> r1;
        std::optional<RootEventIndex> dl0_index;
        std::optional<RootDL0Event> dl0;
        std::optional<RootEventIndex> dl1_index;
        std::optional<RootDL1Event> dl1;
        std::optional<RootPointing> pointing;
        std::optional<RootEventIndex> monitor_index;
        std::optional<RootMonitor> monitor;
        std::vector<std::optional<RootDL2Geometry>> dl2_geometry;
        std::optional<RootDL2Event> dl2;
        std::optional<RootEventIndex> dl2_index;


        
        
};