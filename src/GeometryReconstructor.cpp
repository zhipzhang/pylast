#include "Coordinates.hh"
#include "GeometryReconstructor.hh"
#include "ImageParameters.hh"
#include <unordered_map>


void GeometryReconstructor::operator()(ArrayEvent& event)
{
    if(!event.dl1.has_value())
    {
        throw std::runtime_error("dl1  level event not found");
    }
    if(!event.dl2.has_value())
    {
        throw std::runtime_error("dl2  level event not found");
    }
    std::unordered_map<int, HillasParameter> hillas_dicts;
    telescopes.clear();
    array_pointing_direction = SphericalRepresentation(event.pointing->array_azimuth, event.pointing->array_altitude);
    for(const auto& [tel_id, dl1]: event.dl1->tels)
    {
        if((*query_)(dl1->image_parameters))
        {
            hillas_dicts[tel_id] = dl1->image_parameters.hillas;
            telescope_pointing[tel_id] = SphericalRepresentation(event.pointing->tels[tel_id]->azimuth, event.pointing->tels[tel_id]->altitude);
            telescopes.push_back(tel_id);
        }
    }
    
    if(hillas_dicts.size() < 2)
    {
        // Set dl2 to Is_valid = false
        geometry.is_valid = false;
        event.dl2->geometry[this->name()] = geometry;
        return;
    }
    reconstruct(hillas_dicts);
    auto true_direction = SkyDirection(AltAzFrame(), event.simulation->shower.az, event.simulation->shower.alt);
    auto rec_direction = SkyDirection(AltAzFrame(), geometry.az, geometry.alt);
    geometry.direction_error = true_direction->angle_separation(rec_direction.position);
    event.dl2->geometry[this->name()] = geometry;
}