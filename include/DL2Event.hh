/**
 * @file DL2Event.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief store the information of the DL2 event
 * @version 0.1
 * @date 2025-03-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "ReconstructedGeometry.hh"
#include <unordered_map>
#include <string>
#include "TelImpactParameter.hh"
#include <stdexcept>
class TelReconstructedParameter
{
    public:
        TelReconstructedParameter() = default;
        ~TelReconstructedParameter() = default;
        TelReconstructedParameter(const TelReconstructedParameter& other) = default;
        TelReconstructedParameter& operator=(const TelReconstructedParameter& other) = default;
        TelReconstructedParameter(TelReconstructedParameter&& other) noexcept = default;
        TelReconstructedParameter& operator=(TelReconstructedParameter&& other) noexcept = default;
        
        std::unordered_map<std::string, TelImpactParameter> impact_parameters;
        double estimate_energy = 0;
        double disp = 0;
        
        // Returns the impact parameter value when there's only one entry
        TelImpactParameter& impact() {
            if (impact_parameters.size() == 1) {
                return impact_parameters.begin()->second;
            }
            throw std::runtime_error("Cannot get default impact: multiple impact parameters available");
        }
        
        // Access a specific impact parameter by name
        TelImpactParameter& impact(const std::string& name) {
            return impact_parameters.at(name);
        }
        
        // Const version
        const TelImpactParameter& impact(const std::string& name) const {
            return impact_parameters.at(name);
        }
};





class DL2Event
{
    public:
        DL2Event() = default;
        ~DL2Event() = default;
        DL2Event(const DL2Event& other) = delete;
        DL2Event& operator=(const DL2Event& other) = delete;
        DL2Event(DL2Event&& other) noexcept = default;
        DL2Event& operator=(DL2Event&& other) noexcept = default;
        std::unordered_map<std::string, ReconstructedGeometry> geometry;
        std::unordered_map<int, TelReconstructedParameter> tels;
        std::unordered_map<std::string, ReconstructedEnergy> energy;
        double hadroness = 0;
        void add_energy(std::string name, ReconstructedEnergy energy)
        {
            this->energy[name] = energy;
        }
        void add_geometry(std::string name, ReconstructedGeometry geometry)
        {
            this->geometry[name] = geometry;
        }
        void add_tel_geometry(int tel_id, double impact_parameter, std::string geometry_reconstructor_name)
        {
            TelReconstructedParameter tel_reconstructed_parameter;
            tel_reconstructed_parameter.impact_parameters[geometry_reconstructor_name] = TelImpactParameter(impact_parameter, 0);
            tels[tel_id] = tel_reconstructed_parameter;
        }
        void add_tel_geometry(int tel_id, std::vector<double> impact_parameters, std::vector<std::string> reconstructor_names)
        {
            TelReconstructedParameter tel_reconstructed_parameter;
            for(size_t i = 0; i < impact_parameters.size(); i++)
            {
                tel_reconstructed_parameter.impact_parameters[reconstructor_names[i]] = TelImpactParameter(impact_parameters[i], 0);
            }
            tels[tel_id] = tel_reconstructed_parameter;
        }
        void set_tel_estimate_energy(int tel_id, double energy)
        {
            tels[tel_id].estimate_energy = energy;
        }
        void set_tel_disp(int tel_id, double disp)
        {
            tels[tel_id].disp = disp;
        }
};