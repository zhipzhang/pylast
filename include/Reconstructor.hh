/**
 * @file Reconstructor.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Base class for all reconstructors
 * @version 0.1
 * @date 2025-04-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once
#include "ConfigSystem.hh"
#include "ConfigMacros.hh"
#include "ImageParameters.hh"
#include "ImageQuery.hh"
#include "ArrayEvent.hh"
#include "Coordinates.hh"

class Reconstructor: public config::Configurable
{
    public:
        CONFIG_CONSTRUCTORS(Reconstructor);
        virtual ~Reconstructor() = default;
        void registerParams() override;
        void setUp() override;
        virtual void operator()(ArrayEvent& event);
        virtual std::string name() const {return "BaseReconstructor";}
        std::vector<int> telescopes; // Telescopes Pass the Image Query
        SphericalRepresentation array_pointing_direction;
        static double compute_angle_separation(double az1, double alt1, double az2, double alt2);
    protected:
        std::unique_ptr<ImageQuery> query_;
        std::string image_query_config_;
        bool use_fake_hillas = false;
        bool use_gaussian_fit = false;
};
