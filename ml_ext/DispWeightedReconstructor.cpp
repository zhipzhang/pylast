#include "DispWeightedReconstructor.hh"
#include "Eigen/Dense"
#include "GeometryReconstructor.hh"
#include "ReconstructorFactory.hh"


static bool registered_disp_weighted_reconstructor = []() {
    ReconstructorFactory::instance().register_reconstructor("DispWeightedReconstructor", [](const SubarrayDescription& subarray, const json& config) -> std::unique_ptr<GeometryReconstructor> {
    return std::make_unique<DispWeightedReconstructor>(subarray, config);
});
    return true;
}();

void DispWeightedReconstructor::operator()(ArrayEvent& event)
{
    GeometryReconstructor::operator()(event);
    if(hillas_dicts.size() < 2)
    {
        spdlog::warn("Actually disp method can work less than 2 telescopes, but we skip it for now");
        geometry.is_valid = false;
        event.dl2->geometry[this->name()] = geometry;
        return;
    }
    double rec_alt = event.dl2->geometry["HillasReconstructor"].alt;
    double rec_az = event.dl2->geometry["HillasReconstructor"].az;
    double rec_offset = compute_angle_separation(rec_az, rec_alt, array_pointing_direction.azimuth, array_pointing_direction.altitude) * 180 / M_PI;
    auto [true_x, true_y] = convert_to_fov(event.simulation->shower.alt, event.simulation->shower.az);
    auto [rec_x, rec_y] = convert_to_fov(rec_alt, rec_az);
    spdlog::info("True direction:{}, {}", true_x, true_y);
    spdlog::info("Initial reconstruction: {}, {}", rec_x, rec_y);
    spdlog::info("Initial offset: {}", rec_offset);
    
    std::vector<Eigen::Matrix2d> transform_matrix(telescopes.size());
    for(int i = 0; i < telescopes.size(); i++)
    {
        int tel_id = telescopes[i];
        transform_matrix[i] = Eigen::Matrix2d::Identity();
        transform_matrix[i](0, 0) = std::cos(hillas_dicts.at(tel_id).psi);
        transform_matrix[i](0, 1) = -std::sin(hillas_dicts.at(tel_id).psi);
        transform_matrix[i](1, 0) = std::sin(hillas_dicts.at(tel_id).psi);
        transform_matrix[i](1, 1) = std::cos(hillas_dicts.at(tel_id).psi);
    }
    
    int max_iteration = 6;
    for(int i = 0; i < max_iteration; i++)
    {
        Eigen::VectorXd disp_values = Eigen::VectorXd::Zero(telescopes.size());
        Eigen::VectorXd predict_x = Eigen::VectorXd::Zero(telescopes.size());
        Eigen::VectorXd predict_y = Eigen::VectorXd::Zero(telescopes.size());
        Eigen::Matrix2d J = Eigen::Matrix2d::Zero();
        Eigen::Vector2d y = Eigen::Vector2d::Zero();
        for(int j = 0; j < telescopes.size(); j++)
        {
            int tel_id = telescopes[j];
            double beta_err = sbeta_estimator.predict(rec_offset, const_cast<const ArrayEvent&>(event), tel_id);
            double cog_err = scog_estimator.predict(rec_offset, const_cast<const ArrayEvent&>(event), tel_id);
            double disp_err = sdisp_estimator.predict(rec_offset, const_cast<const ArrayEvent&>(event), tel_id);
            double disp_value = disp_estimator.predict(rec_offset, const_cast<const ArrayEvent&>(event), tel_id);
            double disp_distance = sqrt(pow(rec_x - hillas_dicts.at(tel_id).x, 2) + pow(rec_y - hillas_dicts.at(tel_id).y, 2));
            disp_values(j) = disp_value;
            double var_perp = std::pow(disp_distance * beta_err, 2) + std::pow(cog_err, 2); 
            double var_parallel      = std::pow(disp_err, 2);                                        
            Eigen::Matrix2d Sigma_local = Eigen::Matrix2d::Zero();
            Sigma_local(0,0) = var_parallel;
            Sigma_local(1,1) = var_perp;
            // We use the rec_x, rec_y to decided the sign.
            if(rec_x > hillas_dicts.at(tel_id).x)
            {
                predict_x(j) = hillas_dicts.at(tel_id).x + fabs(disp_value * std::cos(hillas_dicts.at(tel_id).psi));
            }
            else
            {
                predict_x(j) = hillas_dicts.at(tel_id).x - fabs(disp_value * std::cos(hillas_dicts.at(tel_id).psi));
            }
            if(rec_y > hillas_dicts.at(tel_id).y)
            {
                predict_y(j) = hillas_dicts.at(tel_id).y + fabs(disp_value * std::sin(hillas_dicts.at(tel_id).psi));
            }
            else
            {
                predict_y(j) = hillas_dicts.at(tel_id).y - fabs(disp_value * std::sin(hillas_dicts.at(tel_id).psi));
            }
            Eigen::Matrix2d Sigma_a = transform_matrix[j] * Sigma_local * transform_matrix[j].transpose();
            const double eps = 1e-12;
            Sigma_a(0,0) += eps;
            Sigma_a(1,1) += eps;
    
            Eigen::Matrix2d Lambda_a = Sigma_a.ldlt().solve(Eigen::Matrix2d::Identity());
            Eigen::Vector2d X_a(predict_x(j), predict_y(j));
            J += Lambda_a;
            y += Lambda_a * X_a;
  
        }
        if(!use_weight)
        {
            spdlog::warn("No weight is used");
            Eigen::VectorXd weights = Eigen::VectorXd::Zero(telescopes.size());
            for(int k = 0; k < telescopes.size(); k++)
            {
                int tel_id = telescopes[k];
                weights(k) = hillas_dicts[tel_id].intensity;
            }
            double x = predict_x.dot(weights) / weights.sum();
            double y = predict_y.dot(weights) / weights.sum();
            auto [rec_az, rec_alt] = convert_to_sky(x, y);
            geometry.az = rec_az;
            geometry.alt = rec_alt;
            geometry.is_valid = true;
            geometry.telescopes = telescopes;
            geometry.direction_error = compute_angle_separation(rec_az, rec_alt,event.simulation->shower.az, event.simulation->shower.alt);
            event.dl2->add_geometry(this->name(), geometry);
            for(int j = 0; j < telescopes.size(); j++)
            {
                int tel_id = telescopes[j];
                event.dl2->set_tel_disp(tel_id, disp_values(j));
            }
            return;
        }


        Eigen::Vector2d X = J.ldlt().solve(y);
        if(fabs(X(0) - rec_x) < 1e-6 && fabs(X(1) - rec_y) < 1e-6 || i == max_iteration - 1)
        {
            rec_x = X(0);
            rec_y = X(1);
            auto [rec_az, rec_alt] = convert_to_sky(rec_x, rec_y);
            geometry.az = rec_az;
            geometry.alt = rec_alt;
            geometry.is_valid = true;
            geometry.telescopes = telescopes;
            geometry.direction_error = compute_angle_separation(rec_az, rec_alt,event.simulation->shower.az, event.simulation->shower.alt);
            event.dl2->add_geometry(this->name(), geometry);
            for(int j = 0; j < telescopes.size(); j++)
            {
                int tel_id = telescopes[j];
                double disp_value = disp_values(j);
                event.dl2->set_tel_disp(tel_id, disp_value);
            }
            break;
        }
        rec_x = X(0);
        rec_y = X(1);
        auto [tmp_az, tmp_alt] = convert_to_sky(rec_x, rec_y);
        rec_offset = compute_angle_separation(tmp_az, tmp_alt, array_pointing_direction.azimuth, array_pointing_direction.altitude) * 180 /M_PI;
        spdlog::info("Iteration {}: {}, {}", i, rec_x, rec_y);
    }
}


