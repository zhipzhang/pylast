#pragma once
#include "ErrorLGBMEstimator.hh"
#include "ParameterSchema.hh"
#include "nlohmann_json/json.hpp"
#include "spdlog/spdlog.h"
#include <fstream>
using json = nlohmann::json;

class ParameterEstimator {
public:
  ParameterEstimator(const std::string &config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open file: " + config_path);
    }
    json config = json::parse(file);
    spdlog::info("Loading ParameterEstimator from file: {}", config_path);
    const std::string base_directory_ =
        config.contains("base_dir") ? config.at("base_dir").get<std::string>()
                                    : "";
    const std::string &model_path =
        base_directory_ + config.at("model_file").get<std::string>();
    predictor_ = std::make_unique<LGBMPredictor>(model_path);
    if (config.contains("features")) {
      feature_extractor_ =
          std::make_unique<TelFeatureExtractor>(config.at("features"));
    } else {
      throw std::runtime_error("Features not found in config");
    }
  }
  ParameterEstimator(const std::string &base_directory, const json &config) {
    if (!config.contains("features")) {
      throw std::runtime_error("Features not found in config");
    }
    feature_extractor_ =
        std::make_unique<TelFeatureExtractor>(config.at("features"));
    const std::string &model_path =
        base_directory + config.at("model_file").get<std::string>();
    predictor_ = std::make_unique<LGBMPredictor>(model_path);
  }
  ParameterEstimator(const ParameterEstimator &other) = delete;
  ParameterEstimator(ParameterEstimator &&other) noexcept = default;
  ParameterEstimator &operator=(const ParameterEstimator &other) = delete;
  ParameterEstimator &operator=(ParameterEstimator &&other) noexcept = default;
  ~ParameterEstimator() = default;
  double predict(const ArrayEvent &event, int tel_id) const {
    std::vector<double> features =
        feature_extractor_->extract_tel_features(event, tel_id);
    return predictor_->predict(features);
  }

private:
  std::unique_ptr<TelFeatureExtractor> feature_extractor_;
  std::unique_ptr<LGBMPredictor> predictor_;
};