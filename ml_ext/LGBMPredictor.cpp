#include "LGBMPredictor.hh"
#include <filesystem>

LGBMPredictor::LGBMPredictor(const std::string& model_path) {
   // Check if model_path is a valid file path using filesystem
   bool is_file = std::filesystem::exists(model_path) && std::filesystem::is_regular_file(model_path);
   
   int status;
   if (is_file) {
      status = LGBM_BoosterCreateFromModelfile(model_path.c_str(), &num_total_model_, &booster_);
   } else {
      status = LGBM_BoosterLoadModelFromString(model_path.c_str(), &num_total_model_, &booster_);
   }
   
   if (status != 0) {
      throw std::runtime_error("Failed to load booster: " + std::string(LGBM_GetLastError()));
   }
}

LGBMPredictor::~LGBMPredictor() {
    LGBM_BoosterFree(booster_);
}


double LGBMPredictor::predict(const std::vector<double>& features) {
    double prediction;
    int64_t out_len;
    int status = LGBM_BoosterPredictForMatSingleRow(booster_, features.data(), C_API_DTYPE_FLOAT64, features.size(), 1, C_API_PREDICT_NORMAL, 0, -1, "", &out_len, &prediction);
    if (status != 0) {
        throw std::runtime_error("Failed to predict");
    }
    return prediction;
}