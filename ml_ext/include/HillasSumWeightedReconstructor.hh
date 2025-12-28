#pragma once
#include "GeometryReconstructor.hh"
#include "OffSetParameterEstimator.hh"
#include "ParameterSigmaEstimator.hh"



class HillasSumWeightedReconstructor: public GeometryReconstructor
{
public:
    HillasSumWeightedReconstructor(const SubarrayDescription& subarray, const json& config): GeometryReconstructor(subarray, config){
        spdlog::info("Initializing HillasSumWeightedReconstructor");
        if(!config.contains("beta_estimator"))
        {
            throw std::runtime_error("beta_estimator is not set");
        }
        if(!config.contains("cog_estimator"))
        {
            throw std::runtime_error("cog_estimator is not set");
        }
        sbeta_estimator = OffsetParameterEstimator<ParameterSigmaEstimator>(config["beta_estimator"].get<std::string>());
        scog_estimator = OffsetParameterEstimator<ParameterSigmaEstimator>(config["cog_estimator"].get<std::string>());
        use_weight = config.contains("use_weight") ? config["use_weight"].get<bool>() : true;
    };
    HillasSumWeightedReconstructor(const SubarrayDescription& subarray, const std::string& config_str): GeometryReconstructor(subarray, config_str){
        json config = json::parse(config_str);
        if(!config.contains("beta_estimator"))
        {
            throw std::runtime_error("beta_estimator is not set");
        }
        if(!config.contains("cog_estimator"))
        {
            throw std::runtime_error("cog_estimator is not set");
        }
        sbeta_estimator = OffsetParameterEstimator<ParameterSigmaEstimator>(config["beta_estimator"].get<std::string>());
        scog_estimator = OffsetParameterEstimator<ParameterSigmaEstimator>(config["cog_estimator"].get<std::string>());
        use_weight = config.contains("use_weight") ? config["use_weight"].get<bool>() : true;
    };
    ~HillasSumWeightedReconstructor() = default; 
    virtual std::string name() const override{ return "HillasSumWeightedReconstructor"; };
    void operator()(ArrayEvent& event) override;
private:
    OffsetParameterEstimator<ParameterSigmaEstimator> sbeta_estimator;
    OffsetParameterEstimator<ParameterSigmaEstimator> scog_estimator;
    bool use_weight = true;
};