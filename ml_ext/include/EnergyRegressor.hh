#pragma once

#include "GeometryReconstructor.hh"
#include "OffSetParameterEstimator.hh"
#include "ParameterEstimator.hh"
#include "ReconstructedGeometry.hh"
#include "Reconstructor.hh"

class EnergyRegressor : public Reconstructor {
public:
  EnergyRegressor(const json &config) : Reconstructor(config) {
    if (!config.contains("energy_regressor")) {
      throw std::runtime_error("energy_regressor is not set");
    }
    if (config.contains("with_offset")) {
      with_offset = config["with_offset"].get<bool>();
    }
    if (with_offset) {
      energy_regressor = OffsetParameterEstimator<ParameterEstimator>(
          config["energy_regressor"].get<std::string>());
    } else {
      energy_estimator = std::make_unique<ParameterEstimator>(
          config["energy_regressor"].get<std::string>());
    }
  }
  EnergyRegressor(const std::string &config_str) : Reconstructor(config_str) {
    json config = json::parse(config_str);
    if (!config.contains("energy_regressor")) {
      throw std::runtime_error("energy_regressor is not set");
    }
    if (config.contains("with_offset")) {
      with_offset = config["with_offset"].get<bool>();
    }
    if (with_offset) {
      energy_regressor = OffsetParameterEstimator<ParameterEstimator>(
          config["energy_regressor"].get<std::string>());
    } else {
      energy_estimator = std::make_unique<ParameterEstimator>(
          config["energy_regressor"].get<std::string>());
    }
  }

  ~EnergyRegressor() = default;
  void operator()(ArrayEvent &event) override;
  std::string name() const override { return "EnergyRegressor"; };

  OffsetParameterEstimator<ParameterEstimator> energy_regressor;
  // Possible that we don't need consider for offset angle.

  std::unique_ptr<ParameterEstimator> energy_estimator;
  ReconstructedEnergy energy_reco;
  bool with_offset = false;
};