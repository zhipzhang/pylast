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
#include "Configurable.hh"
#include "SubarrayDescription.hh"
#include "CoordFrames.hh"
#include "ReconstructedGeometry.hh"
class GeometryReconstructor: private Configurable
{
    public:
        DECLARE_CONFIGURABLE_DEFINITIONS(const SubarrayDescription& , subarray, GeometryReconstructor);
        virtual ~GeometryReconstructor() = default;
        virtual void operator()(ArrayEvent& event);
        virtual std::string name() const {return "BaseGeometryReconstructor";}
        json default_config() const override {return get_default_config();}
        static json get_default_config();
        void configure(const json& config) override;
        std::vector<int> telescopes;
    protected:
        static double compute_angle_separation(double az1, double alt1, double az2, double alt2);
        std::pair<double, double> convert_to_sky(double fov_x, double fov_y);
        std::pair<double, double> convert_to_fov(double alt, double az);
        std::unique_ptr<ImageQuery> query_;
        ReconstructedGeometry geometry;
        SphericalRepresentation array_pointing_direction;
        std::unique_ptr<TelescopeFrame> nominal_frame;
        std::unordered_map<int, SphericalRepresentation> telescope_pointing;
        const SubarrayDescription& subarray;
        std::unordered_map<int, HillasParameter> hillas_dicts;
};