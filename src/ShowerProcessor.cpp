#include "ShowerProcessor.hh"
#include "CoordFrames.hh"
#include "Coordinates.hh"
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
        // Only store the geometry for the telescopes that were used in the reconstruction
        for(const auto tel_id: geometry_reconstructor->telescopes)
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

                auto true_direction = SkyDirection(AltAzFrame(), event.simulation->shower.az, event.simulation->shower.alt);
                auto telescope_frame = TelescopeFrame(SphericalRepresentation(event.pointing->tels[tel_id]->azimuth, event.pointing->tels[tel_id]->altitude));
                auto tillted_frame =TiltedGroundFrame(telescope_frame.pointing_direction);
                auto fov_direction = true_direction.transform_to(telescope_frame);

                auto core_pos = CartesianPoint(event.simulation->shower.core_x, event.simulation->shower.core_y, 0);
                auto tilted_core_pos = core_pos.transform_to_tilted(tillted_frame);
                auto tel_pos = CartesianPoint(subarray.tel_positions.at(tel_id)[0], subarray.tel_positions.at(tel_id)[1], 0);
                auto tilted_tel_pos = tel_pos.transform_to_tilted(tillted_frame);
                double true_psi = std::atan2(tilted_core_pos.y() - tilted_tel_pos.y(), tilted_core_pos.x() - tilted_tel_pos.x());

                auto cog_point = CameraPoint({dl1->image_parameters.hillas.x, dl1->image_parameters.hillas.y});
                auto true_line_direction = Line2D({fov_direction->x(), fov_direction->y()}, {cos(true_psi), sin(true_psi)});

                double cog_err = true_line_direction.distance(cog_point);

                dl1->image_parameters.extra.true_psi = true_psi;
                double beta_err = true_psi - dl1->image_parameters.hillas.psi;
                // Normalize beta_err to be within [-PI/2, PI/2] to keep it close to 0
                while(beta_err > M_PI/2)
                {
                    beta_err -= M_PI;
                }
                while(beta_err < -M_PI/2)
                {
                    beta_err += M_PI;
                }
                dl1->image_parameters.extra.cog_err = cog_err;
                dl1->image_parameters.extra.beta_err = std::abs(beta_err);
                

                // Miss is the distance between the hillas ellipse center and the true direction
                double off_lon = fov_direction->x() - dl1->image_parameters.hillas.x;
                double off_lat = fov_direction->y() - dl1->image_parameters.hillas.y;
                double disp_projection = off_lon * cos(dl1->image_parameters.hillas.psi) + off_lat * sin(dl1->image_parameters.hillas.psi);
                double disp = sqrt(off_lon * off_lon + off_lat * off_lat);
                double miss = sqrt(pow(disp, 2) - pow(disp_projection, 2));
                dl1->image_parameters.extra.miss = miss;
                dl1->image_parameters.extra.disp = disp ;
                dl1->image_parameters.extra.theta = std::asin(miss/disp);
    }
}
