#include "HillasSumWeightedReconstructor.hh"
#include "Eigen/Dense"
#include "GeometryReconstructor.hh"
#include "ReconstructorFactory.hh"


static bool registered_hillas_sum_weighted_reconstructor = []() {
    ReconstructorFactory::instance().register_reconstructor("HillasSumWeightedReconstructor", [](const SubarrayDescription& subarray, const json& config) -> std::unique_ptr<GeometryReconstructor> {
    return std::make_unique<HillasSumWeightedReconstructor>(subarray, config);
});
    return true;
}();

void HillasSumWeightedReconstructor::operator()(ArrayEvent& event)
{
    GeometryReconstructor::operator()(event);
    if(!event.dl2->geometry.contains("HillasReconstructor"))
    {
        spdlog::error("HillasReconsturctor is not avaliable, it's better to have it as initial value for HillasSumweightedReconstructor");
        throw std::runtime_error("HillasReconsturctor is not avaliable, it's better to have it as initial value for HillasSumweightedReconstructor");
    }
    if(!event.dl2->geometry["HillasReconstructor"].is_valid | (hillas_dicts.size() < 2))
    {
        geometry.is_valid = false;
        event.dl2->geometry[this->name()] = geometry;
        return;
    }
    // Use HillasReconstructor as initial value
    double rec_alt = event.dl2->geometry["HillasReconstructor"].alt;
    double rec_az = event.dl2->geometry["HillasReconstructor"].az;
    double rec_offset = compute_angle_separation(rec_az, rec_alt, array_pointing_direction.azimuth, array_pointing_direction.altitude) * 180.0 / M_PI;

    auto [true_x, true_y] = convert_to_fov(event.simulation->shower.alt, event.simulation->shower.az);
    auto [rec_x, rec_y] = convert_to_fov(rec_alt, rec_az);
    spdlog::debug("True direction:{}, {}", true_x, true_y);
    spdlog::debug("Initial reconstruction: {}, {}", rec_x, rec_y);
    spdlog::debug("Initial offset: {}", rec_offset);

    // Assuming line is ax + by + c = 0
    Eigen::VectorXd a = Eigen::VectorXd::Zero(telescopes.size());
    Eigen::VectorXd b = Eigen::VectorXd::Zero(telescopes.size());
    Eigen::VectorXd c = Eigen::VectorXd::Zero(telescopes.size());
    Eigen::VectorXd helper_miss = Eigen::VectorXd::Zero(telescopes.size());
    for(int i = 0; i < telescopes.size(); i++)
    {
        int tel_id = telescopes[i];
        a(i) = -std::sin(hillas_dicts.at(tel_id).psi);
        b(i) = std::cos(hillas_dicts.at(tel_id).psi);
        c(i) = -hillas_dicts.at(tel_id).x * a(i) - hillas_dicts.at(tel_id).y * b(i);
        helper_miss(i) = std::abs(a(i) * true_x + b(i) * true_y + c(i));

    }
    
    int max_iteration = 6;
    for(int i = 0; i < max_iteration; i++)
    {

        Eigen::Matrix2d M = Eigen::Matrix2d::Zero();
        Eigen::Vector2d v = Eigen::Vector2d::Zero();
        Eigen::VectorXd beta_sigma = Eigen::VectorXd::Zero(telescopes.size());
        Eigen::VectorXd cog_sigma = Eigen::VectorXd::Zero(telescopes.size());
        Eigen::VectorXd disp = Eigen::VectorXd::Zero(telescopes.size());
        
        for(int j = 0; j < telescopes.size(); j++)
        {
            double beta_err = sbeta_estimator.predict(rec_offset, const_cast<const ArrayEvent&>(event), telescopes[j]);
            double cog_err = scog_estimator.predict(rec_offset, const_cast<const ArrayEvent&>(event), telescopes[j]);
            beta_sigma(j) = beta_err;
            cog_sigma(j) = cog_err;
            disp(j) = sqrt(pow(rec_x - hillas_dicts.at(telescopes[j]).x, 2) + pow(rec_y - hillas_dicts.at(telescopes[j]).y, 2));
        }

        Eigen::VectorXd miss_sqaure = cog_sigma.array().pow(2) + beta_sigma.array().pow(2) * disp.array().pow(2);
        Eigen::ArrayXd weights = (1.0 / miss_sqaure.array()).max(1e-9);
        if(!use_weight)
        {
            Eigen::ArrayXd help_miss_square = helper_miss.array().pow(2);
            weights = 1.0 / help_miss_square.max(1e-9);
        }
        // Construct the (2, 2) Matrix M
        M(0, 0) = (a.array().pow(2) *weights.array()).sum() + 1e-9;
        M(1, 1) = (b.array().pow(2) * weights.array()).sum() + 1e-9;
        M(0, 1) = (a.array() * b.array() * weights.array()).sum();
        M(1, 0) = M(0, 1);
        v(0) = (a.array() * weights.array() * c.array()).sum();
        v(1) = (b.array() * weights.array() * c.array()).sum();

        Eigen::Vector2d p= M.ldlt().solve(-v);
        if((fabs(p[0]- rec_x) < 1e-6 && fabs(p[1]- rec_y) < 1e-6) || i == max_iteration - 1)
        {
            // It's time to go out of the loop
            rec_x = p[0];
            rec_y = p[1];
            auto [rec_az, rec_alt] = convert_to_sky(rec_x, rec_y);
            auto covariance = M.inverse();
            double x_uncertainty = sqrt(covariance(0, 0));
            double y_uncertainty = sqrt(covariance(1, 1));
            geometry.az = rec_az;
            geometry.alt = rec_alt;
            geometry.alt_uncertainty = x_uncertainty;
            geometry.az_uncertainty = y_uncertainty;
            geometry.is_valid = true;
            geometry.telescopes = telescopes;
            geometry.direction_error = compute_angle_separation(rec_az, rec_alt, event.simulation->shower.az, event.simulation->shower.alt);
            event.dl2->add_geometry(this->name(), geometry);
            spdlog::debug("Final reconstruction: {}, {}", rec_az, rec_alt);
            break;
        }
        rec_x = p[0];
        rec_y = p[1];
        auto [tmp_az, tmp_alt] = convert_to_sky(rec_x, rec_y);
        rec_offset = compute_angle_separation(tmp_az, tmp_alt, array_pointing_direction.azimuth, array_pointing_direction.altitude) * 180.0 / M_PI;
        spdlog::debug("Iteration {}: {}, {}", i, rec_x, rec_y);
    }
}