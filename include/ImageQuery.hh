/**
 * @file SteroQuery.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief To evaluate the expression of stereo analysis
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #pragma once

 #include "Configurable.hh"
 #include "ImageParameters.hh"
 #include "ExprQuery.hh"
 class ImageQuery : public ExprQuery, public Configurable{
    public:
    /**
     * @brief Construct a new Image Query object
     *        There are two ways to construct the object: 1. using a string expression like "hillas_intensity > 100 && hillas_length > 0"
     *                                                    2. using a json config like {"ImageQuery": {"size > 100pe.": "hillas_intensity > 100", "positive length": "hillas_length > 0"}}
     * 
     * @param expr 
     */
    ImageQuery(const std::string& expr):Configurable(expr, [this](const std::string& expr){set_expr(expr);})
    {
        if(!config.empty())
        {
            initialize();
        }
        init_variables();
    } 
    /**
     * @brief Evaluate the image query on the given image parameters
     * 
     * @param image_parameter 
     * @return true 
     * @return false 
     */
    bool operator() (const ImageParameters& image_parameter);


    protected:
        void init_variables();
        void configure(const json& config) override;
        virtual json default_config() const override
        {
            return json();
        }
    private:
        void init_hillas_parameter();
        void init_leakage_parameter();
        void init_morphology_parameter();
        HillasParameter hillas_parameter_;
        LeakageParameter leakage_parameter_;
        double morphology_n_pixels_;
 };
    
    