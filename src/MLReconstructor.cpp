#include "MLReconstructor.hh"
#include "ImageParameters.hh"

json MLReconstructor::get_default_config()
{
    std::string default_config = R"(
    {
        "ImageQuery": "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3"
    })";
    return from_string(default_config);
}

void MLReconstructor::configure(const json& config)
{
    std::string image_query_config = config["ImageQuery"].dump();
    query_ = std::make_unique<ImageQuery>(image_query_config);
    if(config.contains("use_fake_hillas"))
    {
        use_fake_hillas = config["use_fake_hillas"];
    }
}

void MLReconstructor::operator()(ArrayEvent& event)
{
    array_pointing_direction = SphericalRepresentation(event.pointing->array_azimuth, event.pointing->array_altitude);
    telescopes.clear();
    tel_rec_params.clear();
    if(use_fake_hillas)
    {
        for(const auto tel_id: event.simulation->triggered_tels)
        {
            if((*query_)(event.simulation->tels[tel_id]->fake_image_parameters))
            {
                telescopes.push_back(tel_id);
                tel_rec_params[tel_id] = event.simulation->tels[tel_id]->fake_image_parameters;
            }
        }
        return;
    }
    if(!event.dl1.has_value())
    {
        throw std::runtime_error("dl1  level event not found");
    }
    for(const auto& [tel_id, dl1]: event.dl1->tels)
    {
        if((*query_)(dl1->image_parameters))
        {
            telescopes.push_back(tel_id);
            tel_rec_params[tel_id] = dl1->image_parameters;
        }
    }
}