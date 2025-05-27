/**
 * @file ShowerProcessor.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025
 * 
 */


 #pragma once

/**
 * @brief There will be several parts in the shower processor:
 * 1. Geometry Reconstruction
 * 2. Energy Reconstruction
 * 3. Particle Identification
 * 
 */
 #include <memory>
 #include "ArrayEvent.hh"
 #include "Configurable.hh"
#include "SubarrayDescription.hh"
#include "GeometryReconstructor.hh"
#include "EventSource.hh"

//TODO add stereo quality query?
 class ShowerProcessor: public Configurable
 {
    public:
        DECLARE_CONFIGURABLE_DEFINITIONS(const SubarrayDescription& , subarray, ShowerProcessor);
        // Contain the unique_ptr, so we have to delete the copy-constructor
        ~ShowerProcessor() = default;
        ShowerProcessor(const ShowerProcessor&) = delete;
        ShowerProcessor(ShowerProcessor&&) = default;
        ShowerProcessor& operator=(const ShowerProcessor&) = delete;
        ShowerProcessor& operator=(ShowerProcessor&&) = delete;
        void operator()(ArrayEvent& event);
        void configure(const json& config) override;
        json default_config() const override {return get_default_config();}
        static json get_default_config();

    private:
    const SubarrayDescription& subarray;
    std::vector<std::unique_ptr<GeometryReconstructor>> geometry_reconstructors;
 };