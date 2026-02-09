/**
 * @file LGBMModelLoader.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Load the lightgbm_base model in `pylstmlextension`
 * @version 0.1
 * @date 2026-02-05
 * 
 * @copyright Copyright (c) 2026
 * 
 */

 #pragma once
 #include "LGBMPredictor.hh"
 #include "ParameterSchema.hh"
 #include "nlohmann_json/json.hpp"


 class LGBMModelLoader {
 public:
    LGBMModelLoader(const std::string& config_file_path);
    ~LGBMModelLoader() = default;

    bool IsRegression() const;
    bool IsClassification() const;
    bool IsQuantile() const;

    std::string GetModelName() const;
    int GetFeatureNumber() const;
    std::vector<double> extract_features(const ArrayEvent& event, int tel_id) const;
    double predict(const std::vector<double>& features) const;
 private:
    void initialize(const std::string& base_directory, const nlohmann::json& config);
    std::unique_ptr<LGBMPredictor> predictor_;
    std::unique_ptr<TelFeatureExtractor> feature_extractor_;

    // Some meta data
    std::string model_path_;
    std::string model_type;  // Regression, Classfication, Quantile
    std::string model_name;

    nlohmann::json config_;
 };