#pragma once
#include "ArrayEvent.hh"
#include "CoordFrames.hh"
#include "GeometryReconstructor.hh"
#include "OnnxRunner.hh"


class FlowReconstructor: public GeometryReconstructor
{
public:
    FlowReconstructor(const SubarrayDescription& subarray, const json& config): GeometryReconstructor(subarray, config){
        onnx_runner = std::make_unique<PcfOnnxRunner>(config["onnx_model_path"].get<std::string>(), 3, 2);
    };
    FlowReconstructor(const SubarrayDescription& subarray, const std::string& config_str): GeometryReconstructor(subarray, config_str){
        json config = json::parse(config_str);
        onnx_runner = std::make_unique<PcfOnnxRunner>(config["onnx_model_path"].get<std::string>(), 3, 2);
    };
    ~FlowReconstructor() = default;
    void operator()(ArrayEvent& event) override;
    virtual std::string name() const override{ return "FlowReconstructor"; };

    double total_likelihood(double rec_x, double rec_y, double rec_tilted_core_x, double rec_tilted_core_y, double xmax,double log10_energy, ArrayEvent& event);
    
private:
    std::unique_ptr<PcfOnnxRunner> onnx_runner;
    double get_log_likelihood(const Eigen::VectorXf& charge, const Eigen::Matrix<float, -1, 2, Eigen::RowMajor>& pix_pos, const float d, const float xmax, const float log10_energy);
    std::unique_ptr<TiltedGroundFrame> tilted_frame;
    std::unordered_map<int, Point2D> tiled_tel_pos;
};