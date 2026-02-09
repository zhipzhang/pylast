#pragma once

#include "GeometryReconstructor.hh"
#include "LGBMModelLoader.hh"
#include "ReconstructedGeometry.hh"
#include "Reconstructor.hh"

class ParticleClassifier : public Reconstructor {
public:
  ParticleClassifier(const json &config) : Reconstructor(config) {
    initModel(config);
  }

  ParticleClassifier(const std::string &config_str) : Reconstructor(config_str) {
    initModel(json::parse(config_str));
  }

  ~ParticleClassifier() = default;
  void operator()(ArrayEvent &event) override;
  std::string name() const override { return "ParticleClassifier"; };
  std::unique_ptr<LGBMModelLoader> classifier_model_loader;

private:
  void initModel(const json &config) {
    if (!config.contains("particle_classifier")) {
      throw std::runtime_error("particle_classifier is not set");
    }
    classifier_model_loader = std::make_unique<LGBMModelLoader>(
        config["particle_classifier"].get<std::string>());
    if (!classifier_model_loader->IsClassification()) {
      throw std::runtime_error("particle_classifier is not a classification model");
    }
  }
  ReconstructedParticle particle_reco;
};
