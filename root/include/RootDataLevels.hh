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
#include "ArrayEvent.hh"
#include "CameraGeometry.hh"
#include "CameraReadout.hh"
#include "Eigen/src/Core/Matrix.h"
#include "OpticsDescription.hh"
#include "Pointing.hh"
#include "SubarrayDescription.hh"
#include "TString.h"
#include "tree_gemini.hh"
#include "ROOT/RVec.hxx"
#include "TTree.h"
#include "ImageParameters.hh"
#include "ReconstructedGeometry.hh"
#include "SimulatedShower.hh"
#include "TelImpactParameter.hh"
#include <cstdint>
#include <optional>
#include <sys/types.h>
#include <unordered_map>
#include "spdlog/spdlog.h"
#include <iostream>
using namespace ROOT;

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
       
        TTree* initialize_write(const std::string& name, const std::string& title);
        void initialize_read(TTree* tree);
        TTree* index_tree = nullptr;
        bool get_entry(int ientry)
        {
            if(index_tree)
            {
                if(ientry < index_tree->GetEntries())
                {
                    index_tree->GetEntry(ientry);
                    telescopes = *telescopes_ptr;
                    return true;
                }
            }
            return false;
        }
    private:
        RVecI* telescopes_ptr = nullptr;
};

/**
 * @brief Structure for simulation shower data
 */
class RootSimulationShower 
{
   public:
       int event_id;
       SimulatedShower shower;
       void initialize_write(TTree* tree)
       {
              tree->Branch("event_id", &event_id);
              TTreeSerializer::branch(tree, shower);
       }
       void initialize_read(TTree* tree)
       {
              read_tree = tree;
              tree->SetBranchAddress("event_id", &event_id);
              TTreeSerializer::set_branch_addresses(tree, shower);
       }
       SimulatedShower& get_entry(int ientry)
       {
           if(!read_tree)
           {
               throw std::runtime_error("read_tree is not initialized");
           }
           read_tree->GetEntry(ientry);
           return shower;
       }
       TTree* read_tree = nullptr; // Pointer to the tree for reading
       
};

class CameraGeometryHelper
{
    public:
        std::string camera_name;
        int num_pixels;
        RVecD pix_x;
        RVecD pix_y;
        RVecD pix_area;
        RVecI pix_type;
        double cam_rotation;
};
template<typename TelConfiguration>
class RootTelConfiguration
{
    public:
        int tel_id;
        TelConfiguration config;
        void initialize_write(TTree* tree)
        {
            tree->Branch("tel_id", &tel_id);
            TTreeSerializer::branch(tree, config);
            initialize_internal_structure(tree);
        }
        void initialize_read(TTree* tree)
        {
            read_tree = tree;
            tree->SetBranchAddress("tel_id", &tel_id);
            TTreeSerializer::set_branch_addresses(tree, config);
            initialize_read_pointer(tree);
        }
        virtual ~RootTelConfiguration() = default;
        virtual void initialize_internal_structure(TTree* tree) = 0;
        virtual void initialize_read_pointer(TTree* tree) = 0;
        TTree* read_tree = nullptr;
};

class RootOpticsDescription: public RootTelConfiguration<OpticsDescription>
{
    public:
        TString optics_name;
        void initialize_internal_structure(TTree* tree) override
        {
            tree->Branch("optics_name", &optics_name);
        }
        RootOpticsDescription& operator=(OpticsDescription&& other) noexcept
        {
            config = std::move(other);
            optics_name = TString(config.optics_name.c_str());
            return *this;
        }
        RootOpticsDescription& operator=(const OpticsDescription& other) noexcept
        {
            config = other;
            optics_name = TString(config.optics_name.c_str());
            return *this;
        }
        void initialize_read_pointer(TTree* tree) override
        {
            if(tree->GetBranch("optics_name") != nullptr)
            {
                tree->SetBranchAddress("optics_name", &optics_name_ptr);
            }
        }
        OpticsDescription&& get_entry(int ientry)
        {
            if(read_tree == nullptr)
            {
                throw std::runtime_error("read_tree is not initialized");
            }
            read_tree->GetEntry(ientry);
            config.optics_name = *optics_name_ptr;
            return std::move(config);
        }
    private:
        TString* optics_name_ptr = nullptr;
};

class RootCameraReadout: public RootTelConfiguration<CameraReadout>
{
    public:
        TString camera_name;
        RVecD reference_pulse_shape;
        void initialize_internal_structure(TTree* tree) override
        {
            tree->Branch("camera_name", &camera_name);
            tree->Branch("reference_pulse_shape", &reference_pulse_shape);
        }
        RootCameraReadout& operator=(CameraReadout&& other) noexcept
        {
            config = std::move(other);
            camera_name = TString(config.camera_name.c_str());
            reference_pulse_shape = RVecD(config.reference_pulse_shape.data(), config.reference_pulse_shape.size());
            return *this;
        }
        RootCameraReadout& operator=(const CameraReadout& other) noexcept
        {
            config = other;
            camera_name = TString(config.camera_name.c_str());
            reference_pulse_shape = RVecD(config.reference_pulse_shape.data(), config.reference_pulse_shape.size());
            return *this;
        }
        void initialize_read_pointer(TTree* tree) override
        {
            if(tree->GetBranch("camera_name") != nullptr)
            {
                tree->SetBranchAddress("camera_name", &camera_name_ptr);
            }
            if(tree->GetBranch("reference_pulse_shape") != nullptr)
            {
                tree->SetBranchAddress("reference_pulse_shape", &reference_pulse_shape_ptr);
            }
        }
        CameraReadout&& get_entry(int ientry)
        {
            if(read_tree == nullptr)
            {
                throw std::runtime_error("read_tree is not initialized");
            }
            read_tree->GetEntry(ientry);
            config.camera_name = *camera_name_ptr;
            config.reference_pulse_shape = Eigen::Map<Eigen::VectorXd>(reference_pulse_shape_ptr->data(), reference_pulse_shape_ptr->size());
            return std::move(config);
        }

    private:
        TString* camera_name_ptr = nullptr;
        RVecD* reference_pulse_shape_ptr = nullptr;
};

class RootCameraGeometry: public RootTelConfiguration<CameraGeometryHelper>
{
    public:
        void initialize_internal_structure(TTree* tree) override
        {
            tree->Branch("camera_name", &config.camera_name);
            tree->Branch("pix_x", &config.pix_x);
            tree->Branch("pix_y", &config.pix_y);
            tree->Branch("pix_area", &config.pix_area);
            tree->Branch("pix_type", &config.pix_type);
        }
        
        RootCameraGeometry& operator=(CameraGeometry&& other) noexcept
        {
            config.camera_name = other.camera_name;
            config.cam_rotation = other.cam_rotation;
            config.num_pixels = other.num_pixels;
            config.pix_x = RVecD(other.pix_x.data(), other.pix_x.size());
            config.pix_y = RVecD(other.pix_y.data(), other.pix_y.size());
            config.pix_area = RVecD(other.pix_area.data(), other.pix_area.size());
            config.pix_type = RVecI(other.pix_type.data(), other.pix_type.size());
            return *this;
        }
        RootCameraGeometry& operator=(const CameraGeometry& other) noexcept
        {
            config.camera_name = other.camera_name;
            config.cam_rotation = other.cam_rotation;
            config.num_pixels = other.num_pixels;
            config.pix_x = RVecD(other.pix_x.begin(), other.pix_x.end());
            config.pix_y = RVecD(other.pix_y.begin(), other.pix_y.end());
            config.pix_area = RVecD(other.pix_area.begin(), other.pix_area.end());
            config.pix_type = RVecI(other.pix_type.begin(), other.pix_type.end());
            return *this;
        }
        void initialize_read_pointer(TTree* tree) override
        {
            if(tree->GetBranch("camera_name") != nullptr)
            {
                tree->SetBranchAddress("camera_name", &camera_name_ptr);
            }
            if(tree->GetBranch("pix_x") != nullptr)
            {
                tree->SetBranchAddress("pix_x", &pix_x_ptr);
            }
            if(tree->GetBranch("pix_y") != nullptr)
            {
                tree->SetBranchAddress("pix_y", &pix_y_ptr);
            }
            if(tree->GetBranch("pix_area") != nullptr)
            {
                tree->SetBranchAddress("pix_area", &pix_area_ptr);
            }
            if(tree->GetBranch("pix_type") != nullptr)
            {
                tree->SetBranchAddress("pix_type", &pix_type_ptr);
            }
        }
        CameraGeometry get_entry(int ientry)
        {
            read_tree->GetEntry(ientry);
            Eigen::VectorXd pix_x = Eigen::Map<Eigen::VectorXd>(pix_x_ptr->data(), pix_x_ptr->size());
            Eigen::VectorXd pix_y = Eigen::Map<Eigen::VectorXd>(pix_y_ptr->data(), pix_y_ptr->size());
            Eigen::VectorXd pix_area = Eigen::Map<Eigen::VectorXd>(pix_area_ptr->data(), pix_area_ptr->size());
            Eigen::VectorXi pix_type = Eigen::Map<Eigen::VectorXi>(pix_type_ptr->data(), pix_type_ptr->size());
            return CameraGeometry(
                *camera_name_ptr, 
                config.num_pixels, 
                pix_x, 
                pix_y, 
                pix_area, 
                pix_type, 
                config.cam_rotation
            );
        }
        
    private:
        std::string * camera_name_ptr = nullptr;
        RVecD* pix_x_ptr = nullptr;
        RVecD* pix_y_ptr = nullptr;
        RVecD* pix_area_ptr = nullptr;
        RVecI* pix_type_ptr = nullptr;
};




template<typename DataLevels>
class NewRootDataLevels
{
    public:
        DataLevels datalevels;
        int event_id;
        int tel_id;

        virtual ~NewRootDataLevels() = default;
        virtual void initialize_internal_structure(TTree* tree) = 0;
        virtual void initialize_read_pointer(TTree* tree) = 0;
        virtual void update_after_get_entry() = 0;

        int compute_entry_number(int event_id, int tel_id)
        {
            if(!read_tree)
            {
                throw std::runtime_error("read_tree is not initialized");
            }
            return read_tree->GetEntryNumberWithIndex(event_id, tel_id);
        }
        void initialize_write(TTree* tree)
        {
            if(tree == nullptr)
            {
                throw std::runtime_error("Tree is null");
            }
            tree->Branch("event_id", &event_id);
            tree->Branch("tel_id", &tel_id);
            TTreeSerializer::branch(tree, datalevels);
            initialize_internal_structure(tree);
        }
        void initialize_read(TTree* tree)
        {
            if(tree == nullptr)
            {
                throw std::runtime_error("Tree is null");
            }
            read_tree = tree;
            tree->SetBranchAddress("event_id", &event_id);
            tree->SetBranchAddress("tel_id", &tel_id);
            TTreeSerializer::set_branch_addresses(tree, datalevels);
            initialize_read_pointer(tree);
        }
        DataLevels& get_entry(int ientry)
        {
            if(!read_tree)
            {
                throw std::runtime_error("read_tree is not initialized");
            }
            read_tree->GetEntry(ientry);
            update_after_get_entry();
            return datalevels;
        }
        TTree* read_tree = nullptr;
};

class RootSimulatedCamera: public NewRootDataLevels<SimulatedCamera>
{
    public:
        RVecI true_image;
        RVecD fake_image;
        RVecB fake_image_mask; 
        ImageParameters fake_image_parameters;

        RootSimulatedCamera& operator=(SimulatedCamera&& other) noexcept
        {
            datalevels = std::move(other);
            true_image = std::move(RVecI(datalevels.true_image.data(), datalevels.true_image.size()));
            fake_image = std::move(RVecD(datalevels.fake_image.data(), datalevels.fake_image.size()));
            fake_image_mask = std::move(RVecB(datalevels.fake_image_mask.data(), datalevels.fake_image_mask.size()));
            return *this;
        }
        void initialize_internal_structure(TTree* tree) override
        {
            //tree->Branch("true_image", &true_image);
            //tree->Branch("fake_image", &fake_image);
            //tree->Branch("fake_image_mask", &fake_image_mask);
            TTreeSerializer::branch(tree, fake_image_parameters.hillas, "hillas");
            TTreeSerializer::branch(tree, fake_image_parameters.leakage, "leakage");
            TTreeSerializer::branch(tree, fake_image_parameters.concentration, "concentration");
            TTreeSerializer::branch(tree, fake_image_parameters.morphology, "morphology");
            TTreeSerializer::branch(tree, fake_image_parameters.intensity, "intensity");
            TTreeSerializer::branch(tree, fake_image_parameters.extra, "extra");
        }
        void initialize_read_pointer(TTree* tree) override
        {
            if(tree->GetBranch("true_image") != nullptr)
            {
                tree->SetBranchAddress("true_image", &true_image_ptr);
            }
            if(tree->GetBranch("fake_image") != nullptr)
            {
                tree->SetBranchAddress("fake_image", &fake_image_ptr);
            }
            if(tree->GetBranch("fake_image_mask") != nullptr)
            {
                tree->SetBranchAddress("fake_image_mask", &fake_image_mask_ptr);
            }
            TTreeSerializer::set_branch_addresses(tree, fake_image_parameters.hillas, "hillas");
            TTreeSerializer::set_branch_addresses(tree, fake_image_parameters.leakage, "leakage");
            TTreeSerializer::set_branch_addresses(tree, fake_image_parameters.concentration, "concentration");
            TTreeSerializer::set_branch_addresses(tree, fake_image_parameters.morphology, "morphology");
            TTreeSerializer::set_branch_addresses(tree, fake_image_parameters.intensity, "intensity");
            TTreeSerializer::set_branch_addresses(tree, fake_image_parameters.extra, "extra");
        }
        void update_after_get_entry() override
        {
            if(true_image_ptr)
            {
                datalevels.true_image = Eigen::Map<Eigen::VectorXi>(true_image_ptr->data(), true_image_ptr->size());
            }
            if(fake_image_ptr)
            {
                datalevels.fake_image = Eigen::Map<Eigen::VectorXd>(fake_image_ptr->data(), fake_image_ptr->size());
            }
            if(fake_image_mask_ptr)
            {
                datalevels.fake_image_mask = Eigen::Map<Eigen::Vector<bool, -1>>(fake_image_mask_ptr->data(), fake_image_mask_ptr->size());
            }
            datalevels.fake_image_parameters = fake_image_parameters;
        }
        
    private:
        RVecI* true_image_ptr = nullptr;
        RVecD* fake_image_ptr = nullptr;
        RVecB* fake_image_mask_ptr = nullptr;

};
class RootR0Camera: public NewRootDataLevels<R0Camera>
{
    public:
        RVec<uint16_t> low_gain_waveform;
        RVec<uint16_t> high_gain_waveform;
        RVec<uint32_t> low_gain_waveform_sum;
        RVec<uint32_t> high_gain_waveform_sum;

        RootR0Camera& operator=(R0Camera&& other) noexcept
        {
            datalevels = std::move(other);
            low_gain_waveform = std::move(RVec<uint16_t>(datalevels.waveform[0].data(), datalevels.n_pixels * datalevels.n_samples));
            high_gain_waveform = std::move(RVec<uint16_t>(datalevels.waveform[1].data(), datalevels.n_pixels * datalevels.n_samples));
            low_gain_waveform_sum = std::move(RVec<uint32_t>(datalevels.waveform_sum[0].data(), datalevels.n_pixels));
            high_gain_waveform_sum = std::move(RVec<uint32_t>(datalevels.waveform_sum[1].data(), datalevels.n_pixels));
            return *this;
        }
        void initialize_internal_structure(TTree* tree) override
        {
            tree->Branch("low_gain_waveform", &low_gain_waveform);
            tree->Branch("high_gain_waveform", &high_gain_waveform);
            tree->Branch("low_gain_waveform_sum", &low_gain_waveform_sum);
            tree->Branch("high_gain_waveform_sum", &high_gain_waveform_sum);
        }
        
        void initialize_read_pointer(TTree* tree) override
        {
            if(tree->GetBranch("low_gain_waveform") != nullptr)
            {
                tree->SetBranchAddress("low_gain_waveform", &low_gain_waveform_ptr);
            }
            if(tree->GetBranch("high_gain_waveform") != nullptr)
            {
                tree->SetBranchAddress("high_gain_waveform", &high_gain_waveform_ptr);
            }
            if(tree->GetBranch("low_gain_waveform_sum") != nullptr)
            {
                tree->SetBranchAddress("low_gain_waveform_sum", &low_gain_waveform_sum_ptr);
            }
            if(tree->GetBranch("high_gain_waveform_sum") != nullptr)
            {
                tree->SetBranchAddress("high_gain_waveform_sum", &high_gain_waveform_sum_ptr);
            }
        }
        void update_after_get_entry() override
        {
            if(low_gain_waveform_ptr)
            {
                datalevels.waveform[0] = Eigen::Map<Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>>(low_gain_waveform_ptr->data(), datalevels.n_pixels, datalevels.n_samples);
            }
            if(high_gain_waveform_ptr)
            {
                datalevels.waveform[1] = Eigen::Map<Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>>(high_gain_waveform_ptr->data(), datalevels.n_pixels, datalevels.n_samples);
            }
            if(low_gain_waveform_sum_ptr)
            {
                datalevels.waveform_sum[0] = Eigen::Map<Eigen::Vector<uint32_t, -1>>(low_gain_waveform_sum_ptr->data(), low_gain_waveform_sum_ptr->size());
            }
            if(high_gain_waveform_sum_ptr)
            {
                datalevels.waveform_sum[1] = Eigen::Map<Eigen::Vector<uint32_t, -1>>(high_gain_waveform_sum_ptr->data(), high_gain_waveform_sum_ptr->size());
            }
        }
    private:
        RVec<uint16_t>* low_gain_waveform_ptr = nullptr;
        RVec<uint16_t>* high_gain_waveform_ptr = nullptr;
        RVec<uint32_t>* low_gain_waveform_sum_ptr = nullptr;
        RVec<uint32_t>* high_gain_waveform_sum_ptr = nullptr;

};

class RootR1Camera: public NewRootDataLevels<R1Camera>
{
    public:
        RVecD waveform;
        RVecI gain_selection;

        RootR1Camera& operator=(R1Camera&& other) noexcept
        {
            datalevels = std::move(other);
            waveform = std::move(RVecD(datalevels.waveform.data(), datalevels.n_pixels * datalevels.n_samples));
            gain_selection = std::move(RVecI(datalevels.gain_selection.data(), datalevels.n_pixels));
            return *this;
        }
        void initialize_internal_structure(TTree* tree) override
        {
            tree->Branch("waveform", &waveform);
            tree->Branch("gain_selection", &gain_selection);
        }
        void initialize_read_pointer(TTree* tree) override
        {
            if(tree->GetBranch("waveform") != nullptr)
            {
                tree->SetBranchAddress("waveform", &waveform_ptr);
            }
            if(tree->GetBranch("gain_selection") != nullptr)
            {
                tree->SetBranchAddress("gain_selection", &gain_selection_ptr);
            }
        }
        void update_after_get_entry() override
        {
            if(waveform_ptr)
            {
                datalevels.waveform = Eigen::Map<Eigen::Matrix<double, -1, -1, Eigen::RowMajor>>(waveform_ptr->data(), datalevels.n_pixels, datalevels.n_samples);
            }
            if(gain_selection_ptr)
            {
                datalevels.gain_selection = Eigen::Map<Eigen::VectorXi>(gain_selection_ptr->data(), gain_selection_ptr->size());
            }
        }
    private:
        RVecD* waveform_ptr = nullptr;
        RVecI* gain_selection_ptr = nullptr;
};

class RootDL0Camera: public NewRootDataLevels<DL0Camera>
{
    public:
        RVecD image;
        RVecD peak_time;

        RootDL0Camera& operator=(DL0Camera&& other) noexcept
        {
            datalevels = std::move(other);
            image = std::move(RVecD(datalevels.image.data(), datalevels.image.size()));
            peak_time = std::move(RVecD(datalevels.peak_time.data(), datalevels.peak_time.size()));
            return *this;
        }
        void initialize_internal_structure(TTree* tree) override
        {
            tree->Branch("image", &image);
            tree->Branch("peak_time", &peak_time);
        }
        void initialize_read_pointer(TTree* tree) override
        {
            if(tree->GetBranch("image") != nullptr)
            {
                tree->SetBranchAddress("image", &image_ptr);
            }
            if(tree->GetBranch("peak_time") != nullptr)
            {
                tree->SetBranchAddress("peak_time", &peak_time_ptr);
            }
        }
        void update_after_get_entry() override
        {
            if(image_ptr)
            { 
                datalevels.image = Eigen::Map<Eigen::VectorXd>(image_ptr->data(), image_ptr->size());
            }
            if(peak_time_ptr)
            {
                datalevels.peak_time = Eigen::Map<Eigen::VectorXd>(peak_time_ptr->data(), peak_time_ptr->size());
            }
        }
    private:
        RVecD* image_ptr = nullptr;
        RVecD* peak_time_ptr = nullptr;
};
class RootDL1Camera: public NewRootDataLevels<DL1Camera>
{
    public:
        RVecF image;
        RVecF peak_time;
        RVecB mask;

        RootDL1Camera& operator=(DL1Camera&& other) noexcept
        {
            datalevels = std::move(other);
            image = std::move(RVecF(datalevels.image.data(), datalevels.image.size()));
            peak_time = std::move(RVecF(datalevels.peak_time.data(), datalevels.peak_time.size()));
            mask = std::move(RVecB(datalevels.mask.data(), datalevels.mask.size()));
            return *this;
        }
        void initialize_internal_structure(TTree* tree) override
        {
            // May not written for image
            //tree->Branch("image", &image);
            //tree->Branch("peak_time", &peak_time);
            //tree->Branch("mask", &mask);
            TTreeSerializer::branch(tree, datalevels.image_parameters.hillas, "hillas");
            TTreeSerializer::branch(tree, datalevels.image_parameters.leakage, "leakage");
            TTreeSerializer::branch(tree, datalevels.image_parameters.concentration, "concentration");
            TTreeSerializer::branch(tree, datalevels.image_parameters.morphology, "morphology");
            TTreeSerializer::branch(tree, datalevels.image_parameters.intensity, "intensity");
            TTreeSerializer::branch(tree, datalevels.image_parameters.extra, "extra");
        }
        void initialize_read_pointer(TTree* tree) override
        {
            if(tree->GetBranch("image") != nullptr)
            {
                tree->SetBranchAddress("image", &image_ptr);
            }
            if(tree->GetBranch("peak_time") != nullptr)
            {
                tree->SetBranchAddress("peak_time", &peak_time_ptr);
            }
            if(tree->GetBranch("mask") != nullptr)
            {
                tree->SetBranchAddress("mask", &mask_ptr);
            }
            TTreeSerializer::set_branch_addresses(tree, datalevels.image_parameters.hillas, "hillas");
            TTreeSerializer::set_branch_addresses(tree, datalevels.image_parameters.leakage, "leakage");
            TTreeSerializer::set_branch_addresses(tree, datalevels.image_parameters.concentration, "concentration");
            TTreeSerializer::set_branch_addresses(tree, datalevels.image_parameters.morphology, "morphology");
            TTreeSerializer::set_branch_addresses(tree, datalevels.image_parameters.intensity, "intensity");
            TTreeSerializer::set_branch_addresses(tree, datalevels.image_parameters.extra, "extra");
        }
        void update_after_get_entry() override
        {
            if(image_ptr)
            {
                datalevels.image = Eigen::Map<Eigen::VectorXf>(image_ptr->data(), image_ptr->size());
            }
            if(peak_time_ptr)
            {
                datalevels.peak_time = Eigen::Map<Eigen::VectorXf>(peak_time_ptr->data(), peak_time_ptr->size());
            }
            if(mask_ptr)
            {
                datalevels.mask = Eigen::Map<Eigen::Vector<bool, -1>>(mask_ptr->data(), mask_ptr->size());
            }
        }
        
    private:
        RVecF* image_ptr = nullptr;
        RVecF* peak_time_ptr = nullptr;
        RVecB* mask_ptr = nullptr;
};

class RootDL2Camera: public NewRootDataLevels<TelReconstructedParameter>
{
    public:
        std::vector<float> distance;
        std::vector<float> distance_error;
        std::vector<std::string> reconstructor_name;

        RootDL2Camera& operator=(TelReconstructedParameter&& other) noexcept
        {
            reconstructor_name.clear();
            distance.clear();
            distance_error.clear();
            datalevels = std::move(other);
            for(const auto& [name, impact] : datalevels.impact_parameters)
            {
                reconstructor_name.push_back(name);
                distance.push_back(impact.distance);
                distance_error.push_back(impact.distance_error);
            }
            return *this;
        }
        RootDL2Camera& operator=(const TelReconstructedParameter& other) noexcept
        {
            distance.clear();
            reconstructor_name.clear();
            distance_error.clear();
            datalevels = other;
            for(const auto& [name, impact] : datalevels.impact_parameters)
            {
                reconstructor_name.push_back(name);
                distance.push_back(impact.distance);
                distance_error.push_back(impact.distance_error);
            }
            return *this;
        }
        void initialize_internal_structure(TTree* tree) override
        {
            tree->Branch("reconstructor_name", &reconstructor_name);
            tree->Branch("distance", &distance);
            tree->Branch("distance_error", &distance_error);
        }
        void initialize_read_pointer(TTree* tree) override
        {
            tree->SetBranchAddress("distance", &distance_ptr);
            tree->SetBranchAddress("distance_error", &distance_error_ptr);
            tree->SetBranchAddress("reconstructor_name", &reconstructor_name_ptr);
        }
        void update_after_get_entry() override
        {
            reconstructor_name = *reconstructor_name_ptr;
            distance = *distance_ptr;
            distance_error = *distance_error_ptr;
            datalevels.impact_parameters.clear();
            for(size_t i = 0; i < reconstructor_name.size(); ++i)
            {
                datalevels.impact_parameters[reconstructor_name[i]] = TelImpactParameter{
                    .distance = distance[i],
                    .distance_error = distance_error[i]
                };
            }
        }
        
    private:
        std::vector<float>* distance_ptr;
        std::vector<float>* distance_error_ptr;
        std::vector<std::string>* reconstructor_name_ptr;

};

template<typename DL2RecParameter>
class RootDL2RecParameter 
{
    public:
        DL2RecParameter parameter;
        int event_id;
        void initialize_write(TTree* tree)
        {
            tree->Branch("event_id", &event_id);
            TTreeSerializer::branch(tree, parameter);
            tree->Branch("telescopes", &parameter.telescopes);
        }
        void initialize_read(TTree* tree)
        {
            tree->SetBranchAddress("event_id", &event_id);
            read_tree = tree;
            TTreeSerializer::set_branch_addresses(tree, parameter);
            tree->SetBranchAddress("telescopes", &telescopes_ptr);
        }
        RootDL2RecParameter& operator=(DL2RecParameter&& other) noexcept
        {
            parameter = std::move(other);
            return *this;
        }
        RootDL2RecParameter& operator=(const DL2RecParameter& other) noexcept
        {
            parameter = other;
            return *this;
        }
        void get_entry(int ientry)
        {
            if(!read_tree)
            {
                throw std::runtime_error("read_tree is not initialized");
            }
            read_tree->GetEntry(ientry);
            parameter.telescopes = *telescopes_ptr;
        }
        TTree* read_tree = nullptr;
        std::vector<int>* telescopes_ptr = nullptr;
};
using RootDL2RecGeometry = RootDL2RecParameter<ReconstructedGeometry>;
using RootDL2RecEnergy = RootDL2RecParameter<ReconstructedEnergy>;
using RootDL2RecParticle = RootDL2RecParameter<ReconstructedParticle>;


class RootTelMonitor: public NewRootDataLevels<TelMonitor>
{
    public:
        RVecD dc_to_pe;
        RVecD pedestals;

        RootTelMonitor& operator=(TelMonitor&& other) noexcept
        {
            datalevels = std::move(other);
            dc_to_pe = std::move(RVecD(datalevels.dc_to_pe.data(), datalevels.n_channels * datalevels.n_pixels));
            pedestals = std::move(RVecD(datalevels.pedestal_per_sample.data(), datalevels.n_channels * datalevels.n_pixels));
            return *this;
        }
        void initialize_internal_structure(TTree* tree) override
        {
            tree->Branch("dc_to_pe", &dc_to_pe);
            tree->Branch("pedestals", &pedestals);
        }
        void initialize_read_pointer(TTree* tree) override
        {
            if(tree->GetBranch("dc_to_pe") != nullptr)
            {
                tree->SetBranchAddress("dc_to_pe", &dc_to_pe_ptr);
            }
            if(tree->GetBranch("pedestals") != nullptr)
            {
                tree->SetBranchAddress("pedestals", &pedestals_ptr);
            }
        }
        void update_after_get_entry() override
        {
            if(dc_to_pe_ptr)
            {
                datalevels.dc_to_pe = Eigen::Map<Eigen::MatrixXd>(dc_to_pe_ptr->data(), datalevels.n_channels, datalevels.n_pixels);
            }
            if(pedestals_ptr)
            {
                datalevels.pedestal_per_sample = Eigen::Map<Eigen::MatrixXd>(pedestals_ptr->data(), datalevels.n_channels, datalevels.n_pixels);
            }
        }
        
    private:
        RVecD* dc_to_pe_ptr = nullptr;
        RVecD* pedestals_ptr = nullptr;
};
class RootPointing
{
    public:
        double array_alt;
        double array_azimuth;
        RVecD tel_alt;
        RVecD tel_azimuth;
        RVecI tel_id;
        int event_id;
        void initialize_write(TTree* tree)
        {
            tree->Branch("event_id", &event_id);
            tree->Branch("array_alt", &array_alt);
            tree->Branch("array_azimuth", &array_azimuth);
            tree->Branch("tel_id", &tel_id);
            tree->Branch("tel_alt", &tel_alt);
            tree->Branch("tel_azimuth", &tel_azimuth);
        }
        RootPointing& operator=(const Pointing& other) noexcept
        {
            tel_id.clear();
            tel_alt.clear();
            tel_azimuth.clear();
            array_alt = other.array_altitude;
            array_azimuth = other.array_azimuth;
            for(const auto& [tid, point] : other.tels)
            {
                tel_id.push_back(tid);
                tel_azimuth.push_back(point->azimuth);
                tel_alt.push_back(point->altitude);
            }
            return *this;
        }
        void initialize_read(TTree* tree)
        {
            read_tree = tree;
            tree->SetBranchAddress("event_id", &event_id);
            tree->SetBranchAddress("array_alt", &array_alt);
            tree->SetBranchAddress("array_azimuth", &array_azimuth);
            tree->SetBranchAddress("tel_id", &tel_id_ptr);
            tree->SetBranchAddress("tel_alt", &tel_alt_ptr);
            tree->SetBranchAddress("tel_azimuth", &tel_azimuth_ptr);
        }
        Pointing get_entry(int ientry)
        {
            if(!read_tree)
            {
                throw std::runtime_error("read_tree is not initialized");
            }
            read_tree->GetEntry(ientry);
            Pointing pointing;
            pointing.array_altitude = array_alt;
            pointing.array_azimuth = array_azimuth;
            for(size_t i = 0; i < tel_id.size(); ++i)
            {
                pointing.add_tel(tel_id[i], PointingTelescope{
                    .azimuth = (*tel_azimuth_ptr)[i],
                    .altitude = (*tel_alt_ptr)[i]
                });
            }
            return pointing;
        }
    private:
        TTree* read_tree = nullptr;
        RVecD* tel_alt_ptr = nullptr;
        RVecD* tel_azimuth_ptr = nullptr;
        RVecI* tel_id_ptr = nullptr;
};


class RootEventHelper
{
    public:
        std::optional<RootSimulationShower> root_simulation_shower;
        std::optional<RootSimulatedCamera> root_simulation_camera;
        std::optional<RootR0Camera> root_r0_camera;
        std::optional<RootR1Camera> root_r1_camera;
        std::optional<RootDL0Camera> root_dl0_camera;
        std::optional<RootDL1Camera> root_dl1_camera;
        std::optional<RootDL2Camera> root_dl2_camera;
        std::unordered_map<std::string, std::optional<RootDL2RecGeometry>> root_dl2_rec_geometry_map;
        std::unordered_map<std::string, std::optional<RootDL2RecEnergy>> root_dl2_rec_energy_map;
        std::unordered_map<std::string, std::optional<RootDL2RecParticle>> root_dl2_rec_particle_map;
        std::optional<RootTelMonitor> root_tel_monitor;
        std::optional<RootPointing> root_pointing;
        std::optional<RootEventIndex> root_event_index;


        ArrayEvent get_event();
        ArrayEvent get_event(int ientry)
        {
            current_entry = ientry;
            return get_event();
        }
        
        template<typename RootDataType, typename EventType>
        void process_tel_data_level(std::optional<RootDataType>& root_data, 
                                   std::optional<EventType>& event_data, 
                                   int tel_id)
        {
            if(root_data.has_value())
            {
                int entry_number = root_data->compute_entry_number(root_event_index->event_id, tel_id);
                if(entry_number >= 0)
                {
                    root_data->get_entry(entry_number);
                    if(!event_data.has_value())
                    {
                        event_data = EventType();
                    }
                    event_data->add_tel(tel_id, std::move(root_data->datalevels));
                }
            }
        }
        
        void process_event_level_data(ArrayEvent& event);
        void process_tel_level_data(ArrayEvent& event, int tel_id);
        void process_dl2_rec_data(ArrayEvent& event);
        
        template<typename RootDL2Type, typename DL2ParameterType>
        void process_dl2_rec_data_map(std::unordered_map<std::string, std::optional<RootDL2Type>>& root_map,
                                     std::unordered_map<std::string, DL2ParameterType>& event_map)
        {
            for (auto& [name, root_data] : root_map)
            {
                if(root_data.has_value())
                {
                    root_data->get_entry(current_entry);
                    event_map[name] = root_data->parameter;
                }
            }
        }
        int current_entry = 0;
    private:
        int max_entries = 0;

};

class RootConfigHelper
{
    public:
        std::optional<RootOpticsDescription> root_optics_description;
        std::optional<RootCameraReadout> root_camera_readout;
        std::optional<RootCameraGeometry> root_camera_geometry;
        TelescopeDescription get_telescope_description(int ientry);
};