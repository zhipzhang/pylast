#include "ShowerProcessor.hh"
#include "CoordFrames.hh"
#include "Coordinates.hh"
#include "DL2Event.hh"
#include "HillasReconstructor.hh"
#include "Utils.hh"
#include "spdlog/spdlog.h"
#include "ReconstructorFactory.hh"

void ShowerProcessor::registerParams()
{
    // Register the geometry reconstruction types parameter
    std::vector<std::string> default_types = {"HillasReconstructor"};
    registerParam<std::vector<std::string>>("GeometryReconstructionTypes", default_types, geometry_types_);
}

void ShowerProcessor::setUp()
{
    try {
        const auto& config = getConfig();
        auto cfg = config.contains("ShowerProcessor") ? config["ShowerProcessor"] : config;
        for(const auto& geometry_type : geometry_types_)
        {
            spdlog::info("Checking if geometry reconstruction type {} is registered", geometry_type);
            if (ReconstructorFactory::instance().is_registered(geometry_type))
            {
                reconstructors.push_back(ReconstructorFactory::instance().create(geometry_type, subarray, cfg[geometry_type]));
            }
            else
            {
                spdlog::error("Unknown geometry reconstruction type: {}", geometry_type);
            }
        }
    }
    catch(const std::exception& e) {
        throw std::runtime_error("Error configuring ShowerProcessor: " + std::string(e.what()));
    }
}


void ShowerProcessor::operator()(ArrayEvent& event)
{
    if(!event.dl2)
    {
        event.dl2 = DL2Event();
    }
    for(auto& reconstructor: reconstructors)
    {
        (*reconstructor)(event);
        // Only store the geometry for the telescopes that were used in the reconstruction
        if(reconstructor->name() != "HillasReconstructor")
        {
            continue;
        }
        for(const auto& tel_id: reconstructor->telescopes)
        {
            if(event.dl2->geometry[reconstructor->name()].is_valid)
            {
                auto tel_coord = subarray.tel_positions.at(tel_id);
                std::array<double, 3> rec_core = {event.dl2->geometry[reconstructor->name()].core_x, event.dl2->geometry[reconstructor->name()].core_y, 0};
                auto rec_direction = SkyDirection(AltAzFrame(), event.dl2->geometry[reconstructor->name()].az, event.dl2->geometry[reconstructor->name()].alt)->transform_to_cartesian();
                std::array<double, 3> line_direction = {rec_direction.direction[0], rec_direction.direction[1], rec_direction.direction[2]};
                auto impact_parameter = Utils::point_line_distance(tel_coord, rec_core, line_direction);
                event.dl2->add_tel_geometry(tel_id, impact_parameter, reconstructor->name());
            }
        }
    }

    //TODO Combine the DL1 and Simulation together.
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
                

                // Miss is the distance between the hillas ellipse center and the true direction
                double off_lon = fov_direction->x() - dl1->image_parameters.hillas.x;
                double off_lat = fov_direction->y() - dl1->image_parameters.hillas.y;
                double disp_projection = off_lon * cos(dl1->image_parameters.hillas.psi) + off_lat * sin(dl1->image_parameters.hillas.psi);
                double disp = sqrt(off_lon * off_lon + off_lat * off_lat);
                double miss = sqrt(pow(disp, 2) - pow(disp_projection, 2));
                if(true_psi != M_PI/2 && std::tan(true_psi) * (-off_lon) + off_lat < 0)
                {
                    cog_err = -cog_err;
                }
                if(dl1->image_parameters.hillas.psi != M_PI/2 && std::tan(dl1->image_parameters.hillas.psi) * (off_lon) - off_lat < 0)
                {
                    miss = -miss;
                }
                dl1->image_parameters.extra.cog_err = cog_err;
                dl1->image_parameters.extra.beta_err = beta_err;
                dl1->image_parameters.extra.miss = miss;
                dl1->image_parameters.extra.disp = disp_projection ;
                dl1->image_parameters.extra.theta = std::asin(miss/disp);
    }

    for(auto& [tel_id, simulated_camera]: event.simulation->tels)
    {
        auto& image_parameter = simulated_camera->image_parameters;
        if(image_parameter.hillas.intensity < 40)
        {
            continue;
        }
        auto true_direction = SkyDirection(AltAzFrame(), event.simulation->shower.az, event.simulation->shower.alt);
        auto telescope_frame = TelescopeFrame(SphericalRepresentation(event.pointing->tels[tel_id]->azimuth, event.pointing->tels[tel_id]->altitude));
        auto fov_direction = true_direction.transform_to(telescope_frame);
        auto core_pos = CartesianPoint(event.simulation->shower.core_x, event.simulation->shower.core_y, 0);
        auto tel_pos = CartesianPoint(subarray.tel_positions.at(tel_id)[0], subarray.tel_positions.at(tel_id)[1], 0);
        auto tilted_frame = TiltedGroundFrame(telescope_frame.pointing_direction);
        auto tilted_core_pos = core_pos.transform_to_tilted(tilted_frame);
        auto tilted_tel_pos = tel_pos.transform_to_tilted(tilted_frame);
        double true_psi = std::atan2(tilted_core_pos.y() - tilted_tel_pos.y(), tilted_core_pos.x() - tilted_tel_pos.x());
        auto cog_point = CameraPoint({image_parameter.hillas.x, image_parameter.hillas.y});
        auto true_line_direction = Line2D({fov_direction->x(), fov_direction->y()}, {cos(true_psi), sin(true_psi)});
        double cog_err = true_line_direction.distance(cog_point);
        image_parameter.extra.true_psi = true_psi;
        double beta_err = true_psi - image_parameter.hillas.psi;
        // Normalize beta_err to be within [-PI/2, PI/2] to keep it close to 0
        while(beta_err > M_PI/2)
        {
            beta_err -= M_PI;
        }
        while(beta_err < -M_PI/2)
        {
            beta_err += M_PI;
        }
        // Miss is the distance between the hillas ellipse center and the true direction
        double off_lon = fov_direction->x() - image_parameter.hillas.x;
        double off_lat = fov_direction->y() - image_parameter.hillas.y;
        if(true_psi != M_PI/2 && std::tan(true_psi) * (-off_lon) + off_lat < 0)
        {
            cog_err = -cog_err;
        }
        double disp_projection = off_lon * cos(image_parameter.hillas.psi) + off_lat * sin(image_parameter.hillas.psi);
        double disp = sqrt(off_lon * off_lon + off_lat * off_lat);
        double miss = sqrt(pow(disp, 2) - pow(disp_projection, 2));
        if( image_parameter.hillas.psi!=M_PI/2 && std::tan(image_parameter.hillas.psi)  * (off_lon) - off_lat < 0)
        {
            miss = -miss;
        }
        image_parameter.extra.cog_err = cog_err;
        image_parameter.extra.beta_err = beta_err;
        image_parameter.extra.miss = miss;
        image_parameter.extra.disp = disp_projection;
        image_parameter.extra.theta = std::asin(miss/disp);
    }
}
