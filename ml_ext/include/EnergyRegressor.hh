#pragma once

#include "GeometryReconstructor.hh"
#include "Reconstructor.hh"
#include "OffSetParameterEstimator.hh"
#include "ParameterEstimator.hh"
#include "ReconstructedGeometry.hh"




class EnergyRegressor: public Reconstructor
{
    public:
       EnergyRegressor(const json& config): Reconstructor(config){
        if(!config.contains("energy_regressor"))
        {
            throw std::runtime_error("energy_regressor is not set");
        }
        energy_regressor = OffsetParameterEstimator<ParameterEstimator>(config["energy_regressor"].get<std::string>());
       }
       EnergyRegressor(const std::string& config_str): Reconstructor(config_str){
        json config = json::parse(config_str);
        if(!config.contains("energy_regressor"))
        {
            throw std::runtime_error("energy_regressor is not set");
        }
        energy_regressor = OffsetParameterEstimator<ParameterEstimator>(config["energy_regressor"].get<std::string>());
       }

       ~EnergyRegressor() = default;
       void operator()(ArrayEvent& event) override;
       std::string name() const override{ return "EnergyRegressor"; };
       OffsetParameterEstimator<ParameterEstimator> energy_regressor;
       ReconstructedEnergy energy_reco;

};