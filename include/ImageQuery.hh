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
    ImageQuery(const std::string& expr):Configurable(expr, [this](const std::string& expr){
        set_expr(expr);
    })
    {
        if(!config.empty())
        {
            initialize();
        }
        init_variables();
    }
    ~ImageQuery() = default;
    bool operator() (const ImageParameters& image_parameter);
    protected:
        void init_variables() override;
        void configure(const json& config) override;
        virtual json default_config() const override
        {
            return json();
        }
    private:
        void init_hillas_parameter();
        void init_leakage_parameter();
        HillasParameter hillas_parameter_;
        LeakageParameter leakage_parameter_;
 };
    
    