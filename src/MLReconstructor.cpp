#include "MLReconstructor.hh"

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
}

void MLReconstructor::operator()(ArrayEvent& event)
{
    array_pointing_direction = SphericalRepresentation(event.pointing->array_azimuth, event.pointing->array_altitude);
    telescopes.clear();
    if(!event.dl1.has_value())
    {
        throw std::runtime_error("dl1  level event not found");
    }
    for(const auto& [tel_id, dl1]: event.dl1->tels)
    {
        if((*query_)(dl1->image_parameters))
        {
            telescopes.push_back(tel_id);
        }
    }
}