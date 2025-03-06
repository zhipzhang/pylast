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
class GeometryReconstructor
{
    public:
        GeometryReconstructor(const SubarrayDescription& subarray): subarray(subarray) {};
        virtual ~GeometryReconstructor() = default;
        void operator()(ArrayEvent& event) ;
        virtual bool reconstruct(const std::unordered_map<int, HillasParameter>& hillas_dicts) = 0;
        virtual std::string name() const = 0;
    protected:
        ReconstructedGeometry geometry;
        SphericalRepresentation array_pointing_direction;
        std::unordered_map<int, SphericalRepresentation> telescope_pointing;
        const SubarrayDescription& subarray;
        std::unique_ptr<ImageQuery> query_;
        std::vector<int> telescopes;
};