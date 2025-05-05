/**
 * @file HillasReconstructor.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  Traditional Hillas reconstruction
 * @version 0.1
 * @date 2025-03-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #pragma once

 #include "GeometryReconstructor.hh"
#include "ImageParameters.hh"
#include "SubarrayDescription.hh"
#include <cstdint>
#include <unordered_map>

 class HillasReconstructor: public GeometryReconstructor
 {
    public:
        HillasReconstructor(const SubarrayDescription& subarray): GeometryReconstructor(subarray) {};
        HillasReconstructor(const SubarrayDescription& subarray, const json& config): GeometryReconstructor(subarray, config) {};
        HillasReconstructor(const SubarrayDescription& subarray, const std::string& config_str): GeometryReconstructor(subarray, config_str) {};
        virtual ~HillasReconstructor() = default;
        bool reconstruct(const std::unordered_map<int, HillasParameter>& hillas_dicts);
        virtual std::string name() const override{ return "HillasReconstructor"; };
        virtual void operator()(ArrayEvent& event) override;

        std::unordered_map<int, double> impact_parameters;
    private:
        std::unordered_map<int, HillasParameter> nominal_hillas_dicts;
        void fill_nominal_hillas_dicts(const std::unordered_map<int, HillasParameter>& hillas_dicts);
        std::tuple<double, double, double, double> reconstruction_nominal_intersection();
        std::tuple<double, double, double, double> reconstruction_tilted_intersection();
        double reconstruction_hmax(double fov_x, double fov_y,double altitude);
        std::vector<std::pair<int, int>> get_tel_pairs();
        std::unordered_map<int, Point2D> get_tiled_tel_position();
        std::unique_ptr<TiltedGroundFrame> tilted_frame;
        std::pair<double, double> project_to_ground(Eigen::Vector3d intersection_position, const SkyDirection<AltAzFrame>& direction);
        static double knonrad_weight(double reduced_amplitude, double delta_1, double delta_2, double sin_part);
    
    
 };