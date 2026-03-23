#include "ShowerProcessor.hh"
#include "CoordFrames.hh"
#include "Coordinates.hh"
#include "DL2Event.hh"
#include "HillasReconstructor.hh"
#include "Utils.hh"
#include "spdlog/spdlog.h"
#include "ReconstructorFactory.hh"
#include <cmath>

namespace {
/** Compute cog_err, beta_err, miss and related quantities for a given ellipse center and orientation. */
struct CogBetaMissResult {
    double cog_err;
    double beta_err;
    double miss;
    double disp;
    double disp_projection;
    double theta;
};

CogBetaMissResult computeCogBetaMiss(double fov_x, double fov_y, double true_psi,
                                     double cog_x, double cog_y, double psi)
{
    auto cog_point = CameraPoint({cog_x, cog_y});
    auto true_line_direction = Line2D({fov_x, fov_y}, {std::cos(true_psi), std::sin(true_psi)});
    double cog_err = true_line_direction.distance(cog_point);

    double beta_err = true_psi - psi;
    while (beta_err > M_PI / 2)
        beta_err -= M_PI;
    while (beta_err < -M_PI / 2)
        beta_err += M_PI;

    double off_lon = fov_x - cog_x;
    double off_lat = fov_y - cog_y;
    double disp_projection = off_lon * std::cos(psi) + off_lat * std::sin(psi);
    double disp = std::sqrt(off_lon * off_lon + off_lat * off_lat);
    double miss = std::sqrt(disp * disp - disp_projection * disp_projection);

    if (true_psi != M_PI / 2 && std::tan(true_psi) * (-off_lon) + off_lat < 0)
        cog_err = -cog_err;
    if (psi != M_PI / 2 && std::tan(psi) * off_lon - off_lat < 0)
        miss = -miss;

    double theta = (disp > 1e-10) ? std::asin(miss / disp) : 0.0;
    return {cog_err, beta_err, miss, disp, disp_projection, theta};
}
}  // namespace

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
                auto tillted_frame = TiltedGroundFrame(telescope_frame.pointing_direction);
                auto fov_direction = true_direction.transform_to(telescope_frame);

                auto core_pos = CartesianPoint(event.simulation->shower.core_x, event.simulation->shower.core_y, 0);
                auto tilted_core_pos = core_pos.transform_to_tilted(tillted_frame);
                auto tel_pos = CartesianPoint(subarray.tel_positions.at(tel_id)[0], subarray.tel_positions.at(tel_id)[1], 0);
                auto tilted_tel_pos = tel_pos.transform_to_tilted(tillted_frame);
                double true_psi = std::atan2(tilted_core_pos.y() - tilted_tel_pos.y(), tilted_core_pos.x() - tilted_tel_pos.x());

                auto& hillas = dl1->image_parameters.hillas;
                auto result = computeCogBetaMiss(fov_direction->x(), fov_direction->y(), true_psi,
                                                hillas.x, hillas.y, hillas.psi);

                dl1->image_parameters.extra.true_psi = true_psi;
                dl1->image_parameters.extra.cog_err = result.cog_err;
                dl1->image_parameters.extra.beta_err = result.beta_err;
                dl1->image_parameters.extra.miss = result.miss;
                dl1->image_parameters.extra.disp = result.disp_projection;
                dl1->image_parameters.extra.theta = result.theta;
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
        double true_psi = (std::fabs(tilted_core_pos.x() - tilted_tel_pos.x()) < 1e-6)
                             ? M_PI / 2
                             : std::atan2(tilted_core_pos.y() - tilted_tel_pos.y(),
                                          tilted_core_pos.x() - tilted_tel_pos.x());

        auto& hillas = image_parameter.hillas;
        auto result = computeCogBetaMiss(fov_direction->x(), fov_direction->y(), true_psi,
                                        hillas.x, hillas.y, hillas.psi);

        image_parameter.extra.true_psi = true_psi;
        image_parameter.extra.cog_err = result.cog_err;
        image_parameter.extra.beta_err = result.beta_err;
        image_parameter.extra.miss = result.miss;
        image_parameter.extra.disp = result.disp_projection;
        image_parameter.extra.theta = result.theta;

        auto& two_gauss = image_parameter.two_gaussian_fit;
        auto gaussian_result = computeCogBetaMiss(fov_direction->x(), fov_direction->y(), true_psi,
                                                 two_gauss.mean_x, two_gauss.mean_y, two_gauss.psi);
        two_gauss.cog_err = gaussian_result.cog_err;
        two_gauss.beta_err = gaussian_result.beta_err;
        two_gauss.disp = gaussian_result.disp_projection;
        two_gauss.miss = gaussian_result.miss;
    }
}
