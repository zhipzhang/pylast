#pragma once
#include "ErrorLGBMEstimator.hh"
#include "ParameterSchema.hh"
#include "nlohmann_json/json.hpp"
#include <fstream>
using json = nlohmann::json;


class ParameterSigmaEstimator {
public:
    ParameterSigmaEstimator(const std::string& config_path)
    {
        std::ifstream file(config_path);
        if(!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + config_path);
        }
        json config = json::parse(file);
        auto base_directory_ = config.at("base_dir").get<std::string>();
        error_lgbm_estimator_ = std::make_unique<ErrorLGBMEstimator>(base_directory_, config);
        if(config.contains("features"))
        {
            feature_extractor_ = std::make_unique<TelFeatureExtractor>(config.at("features"));
        }
        else
        {
            throw std::runtime_error("Features not found in config");
        }
    }
    ParameterSigmaEstimator(const std::string& base_directory, const json& config)
    {
        if(!config.contains("features"))
        {
            throw std::runtime_error("Features not found in config");
        }
        feature_extractor_ = std::make_unique<TelFeatureExtractor>(config.at("features"));
        error_lgbm_estimator_ = std::make_unique<ErrorLGBMEstimator>(base_directory, config);
    }
    ParameterSigmaEstimator(const ParameterSigmaEstimator& other) = delete;
    ParameterSigmaEstimator(ParameterSigmaEstimator&& other)noexcept = default;
    ParameterSigmaEstimator& operator=(const ParameterSigmaEstimator& other) = delete;
    ParameterSigmaEstimator& operator=(ParameterSigmaEstimator&& other) noexcept = default;
    ~ParameterSigmaEstimator() = default;
    double predict(const ArrayEvent& event, int tel_id) const
    {
        std::vector<double> features = feature_extractor_->extract_tel_features(event, tel_id);
        return error_lgbm_estimator_->predict(features);
    }
private:
    std::unique_ptr<TelFeatureExtractor> feature_extractor_;
    std::unique_ptr<ErrorLGBMEstimator> error_lgbm_estimator_;
};