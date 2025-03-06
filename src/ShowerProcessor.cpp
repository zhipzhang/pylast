#include "ShowerProcessor.hh"
#include "DL2Event.hh"
#include "HillasReconstructor.hh"
#include "Utils.hh"

void ShowerProcessor::configure(const json& config)
{
    for(auto& geometry_types: config["GeometryReconstructionTypes"])
    {
        if(geometry_types == "HillasReconstructor")
        {
            geometry_reconstructors.push_back(std::make_unique<HillasReconstructor>(subarray, config["HillasReconstructor"]));
        }
    }
}

json ShowerProcessor::get_default_config()
{
    auto default_config = R"({
        "GeometryReconstructionTypes": ["HillasReconstructor"]
    })";
    auto defualt_json = from_string(default_config);
    defualt_json["HillasReconstructor"] = HillasReconstructor::get_default_config();
    return defualt_json;
}

void ShowerProcessor::operator()(ArrayEvent& event)
{
    if(!event.dl2)
    {
        event.dl2 = DL2Event();
    }
    for(auto& geometry_reconstructor: geometry_reconstructors)
    {
        (*geometry_reconstructor)(event);
        for(const auto& tel_id: event.dl2->geometry[geometry_reconstructor->name()].telescopes)
        {
            auto tel_coord = subarray.tel_positions.at(tel_id);
            std::array<double, 3> rec_core = {event.dl2->geometry[geometry_reconstructor->name()].core_x, event.dl2->geometry[geometry_reconstructor->name()].core_y, 0};
            auto rec_direction = SkyDirection(AltAzFrame(), event.dl2->geometry[geometry_reconstructor->name()].az, event.dl2->geometry[geometry_reconstructor->name()].alt)->transform_to_cartesian();
            std::array<double, 3> line_direction = {rec_direction.direction[0], rec_direction.direction[1], rec_direction.direction[2]};
            auto impact_parameter = Utils::point_line_distance(tel_coord, rec_core, line_direction);
            event.dl2->add_tel_geometry(tel_id, impact_parameter, geometry_reconstructor->name());
        }
    }
}
