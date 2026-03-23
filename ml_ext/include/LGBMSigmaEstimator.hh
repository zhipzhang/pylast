/**
 * @file LGBMSigmaEstimator.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Estimator of sigma using LGBM, comntains two LGBMModelLoader
 * @version 0.1
 * @date 2026-02-18
 * 
 * @copyright Copyright (c) 2026
 * 
 */

 #include "LGBMModelLoader.hh"
 #include "nlohmann_json/json.hpp"
 #include <memory>
 class LGBMSigmaEstimator {
 public:
    LGBMSigmaEstimator(const std::string& config_file_path);
    LGBMSigmaEstimator(const std::string& base_directory, const nlohmann::json& config);
    ~LGBMSigmaEstimator() = default;
    double predict(const ArrayEvent& event, int tel_id) const;
 private:
    void check_model_type() const;
    nlohmann::json config_;
    std::unique_ptr<LGBMModelLoader> model_loader_16;
    std::unique_ptr<LGBMModelLoader> model_loader_84;
 };