#pragma once
#include "GeometryReconstructor.hh"
#include "LGBMSigmaEstimator.hh"
#include "LGBMModelLoader.hh"
#include "OffsetEstimator.hh"


class DispWeightedReconstructor: public GeometryReconstructor
{
public:
    DispWeightedReconstructor(const SubarrayDescription& subarray, const json& config): GeometryReconstructor(subarray, config){
        sbeta_estimator = std::make_unique<OffsetEstimator<LGBMSigmaEstimator>>(config["beta_estimator"].get<std::string>());
        scog_estimator = std::make_unique<OffsetEstimator<LGBMSigmaEstimator>>(config["cog_estimator"].get<std::string>());
        sdisp_estimator = std::make_unique<OffsetEstimator<LGBMSigmaEstimator>>(config["disp_estimator"].get<std::string>());
        disp_estimator = std::make_unique<OffsetEstimator<LGBMModelLoader>>(config["disp_predictor"].get<std::string>());
        use_weight = config.contains("use_weight") ? config["use_weight"].get<bool>() : true;
    };
    DispWeightedReconstructor(const SubarrayDescription& subarray, const std::string& config_str): GeometryReconstructor(subarray, config_str){
        json config = json::parse(config_str);
        sbeta_estimator = std::make_unique<OffsetEstimator<LGBMSigmaEstimator>>(config["beta_estimator"].get<std::string>());
        scog_estimator = std::make_unique<OffsetEstimator<LGBMSigmaEstimator>>(config["cog_estimator"].get<std::string>());
        sdisp_estimator = std::make_unique<OffsetEstimator<LGBMSigmaEstimator>>(config["disp_estimator"].get<std::string>());
        disp_estimator = std::make_unique<OffsetEstimator<LGBMModelLoader>>(config["disp_predictor"].get<std::string>());
        use_weight = config.contains("use_weight") ? config["use_weight"].get<bool>() : true;
    };
    ~DispWeightedReconstructor() = default;
    void operator()(ArrayEvent& event) override;
    virtual std::string name() const override{ return "DispWeightedReconstructor"; };

    std::unique_ptr<OffsetEstimator<LGBMSigmaEstimator>> sbeta_estimator;
    std::unique_ptr<OffsetEstimator<LGBMSigmaEstimator>> scog_estimator;
    std::unique_ptr<OffsetEstimator<LGBMSigmaEstimator>> sdisp_estimator;
    std::unique_ptr<OffsetEstimator<LGBMModelLoader>> disp_estimator;
    bool use_weight = true;


};