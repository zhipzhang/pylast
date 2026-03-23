#include "LGBMSigmaEstimator.hh"
#include <fstream>
#include <filesystem>


LGBMSigmaEstimator::LGBMSigmaEstimator(const std::string& config_file_path) {
    std::ifstream file(config_file_path);
    if(!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + config_file_path);
    }
    config_ = nlohmann::json::parse(file);
    auto base_directory = std::filesystem::path(config_file_path).parent_path().string();
    check_model_type();
    model_loader_16 = std::make_unique<LGBMModelLoader>(base_directory, config_.at("quantile_model_16"));
    model_loader_84 = std::make_unique<LGBMModelLoader>(base_directory, config_.at("quantile_model_84"));
    assert(model_loader_16->IsQuantile() && model_loader_84->IsQuantile());
}
LGBMSigmaEstimator::LGBMSigmaEstimator(const std::string& base_directory, const json& config) {
    config_ = config;
    check_model_type();
    model_loader_16 = std::make_unique<LGBMModelLoader>(base_directory, config_.at("quantile_model_16"));
    model_loader_84 = std::make_unique<LGBMModelLoader>(base_directory, config_.at("quantile_model_84"));
    assert(model_loader_16->IsQuantile() && model_loader_84->IsQuantile());
}
void LGBMSigmaEstimator::check_model_type() const {
    if(!config_.contains("meta"))
    {
        throw std::runtime_error("meta is not found in config");
    }
    if(!config_["meta"].contains("model_type"))
    {
        throw std::runtime_error("model_type is not found in meta");
    }
    if(config_["meta"]["model_type"] != "sigma_estimator")
    {
        throw std::runtime_error("model_type is not sigma_estimator");
    }
    if(!config_.contains("quantile_model_16") || !config_.contains("quantile_model_84"))
    {
        throw std::runtime_error("quantile_model_16 and quantile_model_84 are not found in config");
    }
}


double LGBMSigmaEstimator::predict(const ArrayEvent& event, int tel_id) const {
    std::vector<double> features = model_loader_16->extract_features(event, tel_id);
    double quantile16 = model_loader_16->predict(features);
    double quantile84 = model_loader_84->predict(features);
    return (quantile84 - quantile16) / 2;
}