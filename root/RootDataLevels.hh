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
#include "spdlog/spdlog.h"
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
       TTree* data_tree = nullptr;
       std::optional<int> GetEntries() const {return data_tree ? std::optional<int>(data_tree->GetEntries()) : std::nullopt;}
       bool get_entry(int ientry)
       {
           if(data_tree)
           {
               if(ientry < data_tree->GetEntries())
               {
                   data_tree->GetEntry(ientry);
                   return true;
               }
               else
               {
                   return false;
               }
           }
           return false;
       }
       std::optional<int> get_entry_number(int event_id, int tel_id)
       {
            if(data_tree)
            {
                int entry_number = data_tree->GetEntryNumberWithIndex(event_id, tel_id);
                if(entry_number < 0)
                {
                    spdlog::debug("Failed to get entry number for event_id: {} and tel_id: {}", event_id, tel_id);
                    return std::nullopt;
                }
                return std::optional<int>(entry_number);
            }
            return std::nullopt;
       }
    private:
        int ientry = 0;
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
        TTree* index_tree = nullptr;
        bool get_entry(int ientry)
        {
            if(index_tree)
            {
                if(ientry < index_tree->GetEntries())
                {
                    index_tree->GetEntry(ientry);
                    return true;
                }
            }
            return false;
        }
    private:
        RVecI* telescopes_ptr = nullptr;
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
    private:
        RVec<uint16_t>* low_gain_waveform_ptr = nullptr;
        RVec<uint16_t>* high_gain_waveform_ptr = nullptr;
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
    private:
        RVecD* waveform_ptr = nullptr;
        RVecI* gain_selection_ptr = nullptr;
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
    private:
        RVecD* image_ptr = nullptr;
        RVecD* peak_time_ptr = nullptr;
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
       RVecF image;
       RVecF peak_time;
       RVec<bool> mask;
       
       // Image parameters (complete structure)
       ImageParameters params;
    
       virtual TTree* initialize() override;
       TTree* initialize(bool have_image);
       virtual void initialize(TTree* tree) override;
    private:
        RVecF* image_ptr = nullptr;
        RVecF* peak_time_ptr = nullptr;
        RVec<bool>* mask_ptr = nullptr;
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
    private:
       std::string* reconstructor_name_ptr = nullptr;
       std::vector<int>* telescopes_ptr = nullptr;
};

class RootDL2Energy : public RootDataLevels
{
   public:
       RootDL2Energy(const std::string& reconstructor_name) : reconstructor_name(reconstructor_name) {}
       virtual ~RootDL2Energy() = default;
       std::string reconstructor_name;
       int event_id;
       ReconstructedEnergy energy;
       virtual TTree* initialize() override;
       virtual void initialize(TTree* tree) override;
    private:
        std::string* reconstructor_name_ptr = nullptr;
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
       double estimate_energy = 0;
       double estimate_disp = 0;
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
    private:
        std::vector<std::string>* reconstructor_name_ptr = nullptr;
        std::vector<double>* distance_ptr = nullptr;
        std::vector<double>* distance_error_ptr = nullptr;
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
       void clear()
       {
           tel_id.clear();
           tel_az.clear();
           tel_alt.clear();
       }
    private:
        RVecI* tel_id_ptr = nullptr;
        RVecD* tel_az_ptr = nullptr;
        RVecD* tel_alt_ptr = nullptr;
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
    private:
        RVecD* dc_to_pe_ptr = nullptr;
        RVecD* pedestals_ptr = nullptr;
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
        std::optional<RootR0Event> r0;
        std::optional<RootR1Event> r1;
        std::optional<RootDL0Event> dl0;
        std::optional<RootDL1Event> dl1;
        std::optional<RootPointing> pointing;
        std::optional<RootMonitor> monitor;

        std::optional<RootEventIndex> event_index;
        std::unordered_map<std::string, std::optional<RootDL2Geometry>> dl2_geometry_map;
        std::unordered_map<std::string, std::optional<RootDL2Energy>> dl2_energy_map;
        std::optional<RootDL2Event> dl2;
        int test_entries();
        bool has_event() {return current_entry < entries;}
        void load_next_event();
        void fill_tel_entries();
        void get_event(int index)
        {
            if(index >= entries)
            {
                throw std::runtime_error("index out of range");
            }
            current_entry = index;
        }
        RVecI r0_tel_entries;
        RVecI r1_tel_entries;
        RVecI dl0_tel_entries;
        RVecI dl1_tel_entries;
        RVecI monitor_tel_entries;
        RVecI dl2_tel_entries;
    private:
        template<typename T>
        void fill_tel_entries(std::optional<T>& data_level, std::optional<RootEventIndex>& index, RVecI& tel_entries);
        int entries = 0;
        int current_entry = 0;
        
        
};