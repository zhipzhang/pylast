/**
 * @file MLReconstructor.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Base class for all ML reconstructors
 * @version 0.1
 * @date 2025-04-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once
#include "Configurable.hh"
#include "ImageQuery.hh"
#include "ArrayEvent.hh"

class MLReconstructor: public Configurable
{
    public:
        MLReconstructor(const std::string& config_str): Configurable(config_str)
        {
            initialize();
        }
        virtual ~MLReconstructor() = default;
        json default_config() const override {return get_default_config();}
        static json get_default_config();
        void configure(const json& config) override;
        void operator()(ArrayEvent& event);
        std::vector<int> telescopes; // Telescopes Pass the Image Query
    protected:
        std::unique_ptr<ImageQuery> query_;
};
