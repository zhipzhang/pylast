#include "ImageQuery.hh"

void ImageQuery::init_variables()
{
    init_hillas_parameter();
    init_leakage_parameter();
    init_morphology_parameter();
}

void ImageQuery::init_hillas_parameter()
{
    parser_.DefineVar("hillas_length", &hillas_parameter_.length);
    parser_.DefineVar("hillas_width", &hillas_parameter_.width);
    parser_.DefineVar("hillas_psi", &hillas_parameter_.psi);
    parser_.DefineVar("hillas_x", &hillas_parameter_.x);
    parser_.DefineVar("hillas_y", &hillas_parameter_.y);
    parser_.DefineVar("hillas_intensity", &hillas_parameter_.intensity);
}
void ImageQuery::init_leakage_parameter()
{
    parser_.DefineVar("leakage_pixels_width_1", &leakage_parameter_.pixels_width_1);
    parser_.DefineVar("leakage_pixels_width_2", &leakage_parameter_.pixels_width_2);
    parser_.DefineVar("leakage_intensity_width_1", &leakage_parameter_.intensity_width_1);
    parser_.DefineVar("leakage_intensity_width_2", &leakage_parameter_.intensity_width_2);
}
void ImageQuery::init_morphology_parameter()
{
    parser_.DefineVar("morphology_n_pixels", &morphology_n_pixels_);
}

void ImageQuery::configure(const json& config)
{
    try {
        const json& cfg = config.contains("ImageQuery") ? config.at("ImageQuery") : config;
        for (const auto& [key, value] : cfg.items()) {
            add_expr(value);
        }
        set_expr();
    }
    catch(const std::exception& e) {
        throw std::runtime_error("Error configuring StereoQuery: " + std::string(e.what()));
    }
}

bool ImageQuery::operator()(const ImageParameters &image_parameter)
{
    hillas_parameter_ = image_parameter.hillas;
    leakage_parameter_ = image_parameter.leakage;
    morphology_n_pixels_ = image_parameter.morphology.n_pixels;
    try {
        return parser_.Eval();
    }
    catch(mu::Parser::exception_type& e) {
        printf("Error evaluating ImageQuery:\n");
        printf("Message:  %s\n", e.GetMsg().c_str());
        printf("Formula:  %s\n", e.GetExpr().c_str());
        printf("Token:    %s\n", e.GetToken().c_str());
        printf("Position: %d\n", e.GetPos());
        printf("Error code: %d\n", e.GetCode());
        return false;
    }
    catch(const std::exception& e) {
        printf("Unexpected error evaluating StereoQuery: %s\n", e.what());
        return false;
    }
}