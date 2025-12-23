#include "Reconstructor.hh"
#include "ImageParameters.hh"
#include "CoordFrames.hh"

void Reconstructor::registerParams()
{
    // Register the use_fake_hillas parameter
    registerParam<bool>("use_fake_hillas", true, use_fake_hillas);
    registerParam<std::string>("ImageQuery",  "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3", image_query_config_);
}


void Reconstructor::setUp()
{
    query_ = std::make_unique<ImageQuery>(image_query_config_);
}
void Reconstructor::operator()(ArrayEvent& event)
{
    array_pointing_direction = SphericalRepresentation(event.pointing->array_azimuth, event.pointing->array_altitude);
    telescopes.clear();
    if(use_fake_hillas )
    {
        if(!event.simulation.has_value())
        {
            throw std::runtime_error("simulation data is not available");
        }
        for(const auto tel_id: event.simulation->get_ordered_tels())
        {
            if((*query_)(event.simulation->tels[tel_id]->image_parameters))
            {
                telescopes.push_back(tel_id);
            }
        }
        return;
    }
    if(!event.dl1.has_value() && !use_fake_hillas)
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
    if(!event.dl2.has_value())
    {
        event.dl2 = DL2Event();
    }
    return;
}

double Reconstructor::compute_angle_separation(double az1, double alt1, double az2, double alt2)
{
    auto direction1 = SkyDirection(AltAzFrame(), az1, alt1);
    auto direction2 = SkyDirection(AltAzFrame(), az2, alt2);
    return direction1->angle_separation(direction2.position);
}