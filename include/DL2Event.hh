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
        double estimate_energy = 0;
        double estimate_hadroness = 0;
        double estimate_disp = 0;
        std::unordered_map<std::string, TelImpactParameter> impact_parameters;
        
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
        std::unordered_map<std::string, ReconstructedGeometry> geometry;
        std::unordered_map<int, TelReconstructedParameter> tels;
        std::unordered_map<std::string, ReconstructedEnergy> energy;
        std::unordered_map<std::string, ReconstructedParticle> particle;
        void add_tel(int tel_id, TelReconstructedParameter tel_reconstructed_parameter)
        {
            tels[tel_id] = tel_reconstructed_parameter;
        }
        void add_energy(std::string name, ReconstructedEnergy energy)
        {
            this->energy[name] = energy;
        }
        void add_geometry(std::string name, ReconstructedGeometry geometry)
        {
            this->geometry[name] = geometry;
        }
        void add_particle(std::string name, ReconstructedParticle particle)
        {
            this->particle[name] = particle;
        }
        void add_tel_geometry(int tel_id, double impact_parameter, std::string geometry_reconstructor_name)
        {
            TelReconstructedParameter tel_reconstructed_parameter;
            tel_reconstructed_parameter.impact_parameters[geometry_reconstructor_name] = TelImpactParameter{impact_parameter, 0};
            tels[tel_id] = tel_reconstructed_parameter;
        }
        void add_tel_geometry(int tel_id, std::vector<double> impact_parameters, std::vector<std::string> reconstructor_names)
        {
            TelReconstructedParameter tel_reconstructed_parameter;
            for(size_t i = 0; i < impact_parameters.size(); i++)
            {
                tel_reconstructed_parameter.impact_parameters[reconstructor_names[i]] = TelImpactParameter{impact_parameters[i], 0};
            }
            tels[tel_id] = tel_reconstructed_parameter;
        }
        void set_tel_estimate_energy(int tel_id, double energy)
        {
            tels[tel_id].estimate_energy = energy;
        }
        void set_tel_estimate_hadroness(int tel_id, double hadroness)
        {
            tels[tel_id].estimate_hadroness = hadroness;
        }
        void set_tel_disp(int tel_id, double disp)
        {
            tels[tel_id].estimate_disp = disp;
        }
        void set_tel_estimate_disp(std::vector<int> tel_ids, std::vector<double> disps)
        {
            for(size_t i = 0; i < tel_ids.size(); i++)
            {
                tels[tel_ids[i]].estimate_disp = disps[i];
            }
        }
};