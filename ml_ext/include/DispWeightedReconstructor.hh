#pragma once
#include "GeometryReconstructor.hh"
#include "OffSetParameterEstimator.hh"
#include "ParameterSigmaEstimator.hh"
#include "ParameterEstimator.hh"


class DispWeightedReconstructor: public GeometryReconstructor
{
public:
    DispWeightedReconstructor(const SubarrayDescription& subarray, const json& config): GeometryReconstructor(subarray, config){
        sbeta_estimator = OffsetParameterEstimator<ParameterSigmaEstimator>(config["beta_estimator"].get<std::string>());
        scog_estimator = OffsetParameterEstimator<ParameterSigmaEstimator>(config["cog_estimator"].get<std::string>());
        sdisp_estimator = OffsetParameterEstimator<ParameterSigmaEstimator>(config["disp_estimator"].get<std::string>());
        disp_estimator = OffsetParameterEstimator<ParameterEstimator>(config["disp_predictor"].get<std::string>());
        use_weight = config.contains("use_weight") ? config["use_weight"].get<bool>() : true;
    };
    DispWeightedReconstructor(const SubarrayDescription& subarray, const std::string& config_str): GeometryReconstructor(subarray, config_str){
        json config = json::parse(config_str);
        sbeta_estimator = OffsetParameterEstimator<ParameterSigmaEstimator>(config["beta_estimator"].get<std::string>());
        scog_estimator = OffsetParameterEstimator<ParameterSigmaEstimator>(config["cog_estimator"].get<std::string>());
        sdisp_estimator = OffsetParameterEstimator<ParameterSigmaEstimator>(config["disp_estimator"].get<std::string>());
        disp_estimator = OffsetParameterEstimator<ParameterEstimator>(config["disp_predictor"].get<std::string>());
        use_weight = config.contains("use_weight") ? config["use_weight"].get<bool>() : true;
    };
    ~DispWeightedReconstructor() = default;
    void operator()(ArrayEvent& event) override;
    virtual std::string name() const override{ return "DispWeightedReconstructor"; };

    OffsetParameterEstimator<ParameterSigmaEstimator> sbeta_estimator;
    OffsetParameterEstimator<ParameterSigmaEstimator> scog_estimator;
    OffsetParameterEstimator<ParameterSigmaEstimator> sdisp_estimator;
    OffsetParameterEstimator<ParameterEstimator> disp_estimator;
    bool use_weight = true;


};