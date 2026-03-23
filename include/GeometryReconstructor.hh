/**
 * @file GeometryReconstructor.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2025-02-24
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "ImageQuery.hh"
#include "ArrayEvent.hh"
#include "ConfigSystem.hh"
#include "ConfigMacros.hh"
#include "SubarrayDescription.hh"
#include "CoordFrames.hh"
#include "ReconstructedGeometry.hh"
#include "Reconstructor.hh"
class GeometryReconstructor: public Reconstructor
{
    public:
        GeometryReconstructor(const SubarrayDescription& subarray): Reconstructor(), subarray(subarray) {};
        GeometryReconstructor(const SubarrayDescription& subarray, const json& config): Reconstructor(config), subarray(subarray) {};
        GeometryReconstructor(const SubarrayDescription& subarray, const std::string& config_str): Reconstructor(config_str),subarray(subarray) {};
        virtual ~GeometryReconstructor() = default;
        virtual void operator()(ArrayEvent& event) override;
        virtual std::string name() const override {return "BaseGeometryReconstructor";}
        std::unordered_map<int, Point2D> get_tiled_tel_position(const TiltedGroundFrame& tilted_frame);
    protected:
        std::pair<double, double> convert_to_sky(double fov_x, double fov_y);
        std::pair<double, double> convert_to_fov(double alt, double az);
        static std::pair<double, double> project_to_ground(const Eigen::Vector3d& intersection_position, const SkyDirection<AltAzFrame>& direction);
        ReconstructedGeometry geometry;
        std::unique_ptr<TelescopeFrame> nominal_frame;
        std::unordered_map<int, SphericalRepresentation> telescope_pointing;
        const SubarrayDescription& subarray;
        std::unordered_map<int, HillasParameter> hillas_dicts;
        std::unordered_map<int, HillasParameter> rounded_hillas_dicts;
        std::vector<int> rounded_telescopes;
};