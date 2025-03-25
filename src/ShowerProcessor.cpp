#include "ShowerProcessor.hh"
#include "CoordFrames.hh"
#include "DL2Event.hh"
#include "HillasReconstructor.hh"
#include "Utils.hh"
#include "spdlog/spdlog.h"
void ShowerProcessor::configure(const json& config)
{
    try {
        auto cfg = config.contains("ShowerProcessor") ? config["ShowerProcessor"] : config;
        for(auto& geometry_types: cfg["GeometryReconstructionTypes"])
        {
            if(geometry_types == "HillasReconstructor")
            {
                geometry_reconstructors.push_back(std::make_unique<HillasReconstructor>(subarray, cfg["HillasReconstructor"]));
            }
            else
            {
                spdlog::debug("Unknown geometry reconstruction type: {}", geometry_types.dump());
            }
        }
    }
    catch(const std::exception& e) {
        throw std::runtime_error("Error configuring ShowerProcessor: " + std::string(e.what()));
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
        for(const auto& [tel_id, dl1]: event.dl1->tels)
        {
            if(event.dl2->geometry[geometry_reconstructor->name()].is_valid)
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
    for(auto& [tel_id, dl1]: event.dl1->tels)
    {

            if(!dl1->image_parameters.extra.has_value())
            {
                dl1->image_parameters.extra = ExtraParameters();
                auto true_direction = SkyDirection(AltAzFrame(), event.simulation->shower.az, event.simulation->shower.alt);
                auto telescope_frame = TelescopeFrame(SphericalRepresentation(event.pointing->tels[tel_id]->azimuth, event.pointing->tels[tel_id]->altitude));
                auto fov_direction = true_direction.transform_to(telescope_frame);
                // Miss is the distance between the hillas ellipse center and the true direction
                double off_lon = fov_direction->x() - dl1->image_parameters.hillas.x;
                double off_lat = fov_direction->y() - dl1->image_parameters.hillas.y;
                double disp_projection = off_lon * cos(dl1->image_parameters.hillas.psi) + off_lat * sin(dl1->image_parameters.hillas.psi);
                double disp = sqrt(off_lon * off_lon + off_lat * off_lat);
                double miss = sqrt(pow(disp, 2) - pow(disp_projection, 2));
                dl1->image_parameters.extra->miss = miss;
                dl1->image_parameters.extra->disp = disp_projection ;
            }
    }
}
