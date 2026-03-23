#pragma once
#include "LGBMPredictor.hh"
#include "nlohmann_json/json.hpp"


using json = nlohmann::json;

class ErrorLGBMEstimator {
public:


    ErrorLGBMEstimator( const std::string& base_directory, const json& config)
    {
        const std::string& quantile16_path = base_directory + config.at("quantile_16_model_file").get<std::string>();
        const std::string& quantile84_path = base_directory + config.at("quantile_84_model_file").get<std::string>();
        predictor_quantitle16_ = std::make_unique<LGBMPredictor>(quantile16_path);
        predictor_quantitle84_ = std::make_unique<LGBMPredictor>(quantile84_path);
    }
    ~ErrorLGBMEstimator() = default;
    double predict(const std::vector<double>& features)
    {
        double quantile84 = predictor_quantitle84_->predict(features);
        double quantile16 = predictor_quantitle16_->predict(features);
        return (quantile84 - quantile16) / 2;
    }
private:
    std::unique_ptr<LGBMPredictor> predictor_quantitle16_;
    std::unique_ptr<LGBMPredictor> predictor_quantitle84_;
};