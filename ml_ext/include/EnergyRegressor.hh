#pragma once

#include "GeometryReconstructor.hh"
#include "LGBMModelLoader.hh"
#include "ReconstructedGeometry.hh"
#include "Reconstructor.hh"

class EnergyRegressor : public Reconstructor {
public:
  EnergyRegressor(const json &config) : Reconstructor(config) {
    initModel(config);
  }

  EnergyRegressor(const std::string &config_str) : Reconstructor(config_str) {
    initModel(json::parse(config_str));
  }

  ~EnergyRegressor() = default;
  void operator()(ArrayEvent &event) override;
  std::string name() const override { return "EnergyRegressor"; };
  std::unique_ptr<LGBMModelLoader> energy_model_loader;
  ReconstructedEnergy energy_reco;

private:
  void initModel(const json &config) {
    if (!config.contains("energy_regressor")) {
      throw std::runtime_error("energy_regressor is not set");
    }
    energy_model_loader = std::make_unique<LGBMModelLoader>(
        config["energy_regressor"].get<std::string>());
    if (!energy_model_loader->IsRegression()) {
      throw std::runtime_error("energy_regressor is not a regression model");
    }
  }
};