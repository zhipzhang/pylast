#include "LGBMModelLoader.hh"
#include <filesystem>
#include <fstream>
#include <iostream>


bool LGBMModelLoader::IsRegression() const {
    return model_type == "regression";
}

bool LGBMModelLoader::IsClassification() const {
    return model_type == "classification";
}

bool LGBMModelLoader::IsQuantile() const {
    return model_type == "quantile";
}

std::string LGBMModelLoader::GetModelName() const {
    return model_name;
}

int LGBMModelLoader::GetFeatureNumber() const {
    return feature_extractor_->GetFeatureNumber();
}

std::vector<double> LGBMModelLoader::extract_features(const ArrayEvent& event, int tel_id) const {
    return feature_extractor_->extract_tel_features(event, tel_id);
}

double LGBMModelLoader::predict(const std::vector<double>& features) const {
    return predictor_->predict(features);
}

LGBMModelLoader::LGBMModelLoader(const std::string& config_file_path) {
    std::ifstream file(config_file_path);
    if(!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + config_file_path);
    }
    config_ = nlohmann::json::parse(file);
    auto base_directory = std::filesystem::path(config_file_path).parent_path().string();
    initialize(base_directory, config_);
}
void LGBMModelLoader::initialize(const std::string& base_directory, const nlohmann::json& config) {
    if(!config.contains("model_path")) {
        throw std::runtime_error("model_path is not found in config");
    }
    model_path_ = std::filesystem::path(base_directory) / config.at("model_path").get<std::string>();
    if(!std::filesystem::exists(model_path_)) {
        throw std::runtime_error("model_path does not exist: " + model_path_);
    }
    if(!std::filesystem::is_regular_file(model_path_)) {
        throw std::runtime_error("model_path is not a regular file: " + model_path_);
    }
    predictor_ = std::make_unique<LGBMPredictor>(model_path_);

    auto meta_info = config.at("meta");
    if(!meta_info.contains("model_type")) {
        throw std::runtime_error("model_type is not found in config");
    }
    model_type = meta_info.at("model_type").get<std::string>();

    model_name = meta_info.at("name").get<std::string>();

    if(!config.contains("features")) {
        throw std::runtime_error("features is not found in config");
    }
    feature_extractor_ = std::make_unique<TelFeatureExtractor>(config.at("features"));
}