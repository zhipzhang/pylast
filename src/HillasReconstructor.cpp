#include "HillasReconstructor.hh"
#include "CoordFrames.hh"
#include "Coordinates.hh"
#include "Utils.hh"

void HillasReconstructor::fill_nominal_hillas_dicts(const std::unordered_map<int, HillasParameter>& hillas_dicts)
{
    nominal_hillas_dicts.clear();
    for(const auto& [tel_id, hillas]: hillas_dicts)
    {
        HillasParameter nominal_hillas = hillas;
        auto tel_frame = TelescopeFrame(telescope_pointing[tel_id]);
        // Point1 is the center of the image
        auto point1 = SkyDirection(tel_frame, hillas.x, hillas.y);
        // Point2 is along the major axis of the hillas ellipse
        auto point2 = SkyDirection(tel_frame, hillas.x + cos(hillas.psi), hillas.y + sin(hillas.psi));

        auto nominal_center = point1.transform_to(AltAzFrame()).transform_to(*nominal_frame);
        auto nominal_point2 = point2.transform_to(AltAzFrame()).transform_to(*nominal_frame);
        nominal_hillas.x = nominal_center->x();
        nominal_hillas.y = nominal_center->y();
        nominal_hillas.phi = atan2(nominal_center->y(), nominal_center->x());
        nominal_hillas.psi = atan2(nominal_point2->y() - nominal_center->y(), nominal_point2->x() - nominal_center->x());
        nominal_hillas_dicts[tel_id] = nominal_hillas;
    }
}

std::pair<double, double> HillasReconstructor::project_to_ground(Eigen::Vector3d intersection_position, const SkyDirection<AltAzFrame>& direction)
{
    auto direction_vector = direction->transform_to_cartesian();
    // Calculate the intersection point with the ground (z=0)
    // If the direction is parallel to the ground, return the current position
    if (std::abs(direction_vector.direction.z()) < 1e-10) {
        return {intersection_position.x(), intersection_position.y()};
    }
    
    // Calculate how far we need to go to reach z=0
    double t = -intersection_position.z() / direction_vector.direction.z();
    
    // Calculate the ground intersection point
    double ground_x = intersection_position.x() + t * direction_vector.direction.x();
    double ground_y = intersection_position.y() + t * direction_vector.direction.y();
    
    return {ground_x, ground_y};

}
std::vector<std::pair<int, int>> HillasReconstructor::get_tel_pairs()
{
    std::vector<std::pair<int, int>> tel_pairs;
    for(size_t i = 0; i < telescopes.size(); i++)
    {
        for(size_t j = i + 1; j < telescopes.size(); j++)
        {
            tel_pairs.push_back(std::make_pair(telescopes[i], telescopes[j]));
        }
    }
    return tel_pairs;
}
void HillasReconstructor::operator()(ArrayEvent& event)
{
    GeometryReconstructor::operator()(event);
    if(hillas_dicts.size() < 2)
    {
        // Set dl2 to Is_valid = false
        geometry.is_valid = false;
        event.dl2->geometry[this->name()] = geometry;
        return;
    }
    reconstruct(hillas_dicts);
    geometry.direction_error = compute_angle_separation(event.simulation->shower.az, event.simulation->shower.alt, geometry.az, geometry.alt);
    event.dl2->geometry[this->name()] = geometry;
}
bool HillasReconstructor::reconstruct(const std::unordered_map<int, HillasParameter>& hillas_dicts)
{
    if(hillas_dicts.size() < 2)
    {
        return false;
    }
    tilted_frame = std::make_unique<TiltedGroundFrame>(array_pointing_direction);
    fill_nominal_hillas_dicts(hillas_dicts);
    auto [fov_x, fov_y, sigma_x, sigma_y] = reconstruction_nominal_intersection();
    auto [rec_az, rec_alt] = convert_to_sky(fov_x, fov_y);
    auto [tilted_x, tilted_y, tilted_sigma_x, tilted_sigma_y] = reconstruction_tilted_intersection();
    auto tilted_core_position = CartesianPoint(tilted_x, tilted_y, 0);
    auto intersection_position = tilted_core_position.transform_to_ground(*tilted_frame);
    auto [core_x, core_y] = project_to_ground(intersection_position, SkyDirection(AltAzFrame(), rec_az, rec_alt));

    for(const auto tel_id: telescopes)
    {
        auto tel_coord = subarray.tel_positions.at(tel_id);
        auto impact_parameter = Utils::point_line_distance(tel_coord, {core_x, core_y, 0}, {cos(rec_az), sin(rec_az), 0});
        impact_parameters[tel_id] = impact_parameter;
    }

    geometry.is_valid = true;
    geometry.alt = rec_alt;
    geometry.az = rec_az;
    geometry.alt_uncertainty = sigma_x;
    geometry.az_uncertainty = sigma_y;

    geometry.hmax = reconstruction_hmax(rec_alt);
    geometry.core_x = core_x;
    geometry.core_y = core_y;
    geometry.tilted_core_x = tilted_x;
    geometry.tilted_core_y = tilted_y;
    geometry.tilted_core_uncertainty_x = tilted_sigma_x;
    geometry.tilted_core_uncertainty_y = tilted_sigma_y;
    geometry.telescopes = telescopes;
    return true;
}

std::tuple<double, double, double, double> HillasReconstructor::reconstruction_nominal_intersection()
{
    auto tel_pairs = get_tel_pairs();
    std::vector<double> intersection_x;
    std::vector<double> intersection_y;
    std::vector<double> weight;
    for(const auto& [tel_id1, tel_id2]: tel_pairs)
    {
        auto hillas1 = nominal_hillas_dicts[tel_id1];
        auto hillas2 = nominal_hillas_dicts[tel_id2];
        auto line1 = Line2D({hillas1.x, hillas1.y}, {cos(hillas1.psi), sin(hillas1.psi)});
        auto line2 = Line2D({hillas2.x, hillas2.y}, {cos(hillas2.psi), sin(hillas2.psi)});
        auto intersection = line1.intersection(line2);
        if(intersection.has_value())
        {
            intersection_x.push_back(intersection->x());
            intersection_y.push_back(intersection->y());
            auto delta_1 = 1 - hillas1.width / hillas1.length;
            auto delta_2 = 1 - hillas2.width / hillas2.length;
            auto sin_part = sin(hillas1.psi - hillas2.psi);
            auto reduced_amplitude = hillas1.intensity * hillas2.intensity/ (hillas1.intensity + hillas2.intensity);
            weight.push_back(knonrad_weight(reduced_amplitude, delta_1, delta_2, sin_part));
        }
    }
    auto weight_map = Eigen::Map<Eigen::VectorXd>(weight.data(), weight.size());
    auto x_map = Eigen::Map<Eigen::VectorXd>(intersection_x.data(), intersection_x.size());
    auto y_map = Eigen::Map<Eigen::VectorXd>(intersection_y.data(), intersection_y.size());
    auto mean_x = x_map.dot(weight_map) / weight_map.sum(); 
    auto mean_y = y_map.dot(weight_map) / weight_map.sum();
    auto sigma_x = (x_map.array().square() * weight_map.array()).sum() / weight_map.sum() - mean_x * mean_x;
    auto sigma_y = (y_map.array().square() * weight_map.array()).sum() / weight_map.sum() - mean_y * mean_y;
    return std::make_tuple(mean_x, mean_y, sigma_x, sigma_y);
}

std::unordered_map<int, Point2D> HillasReconstructor::get_tiled_tel_position()
{
    std::unordered_map<int, Point2D> tiled_tel_positions;
    for(const auto tel_id: telescopes)
    {
        auto tel_pos = CartesianPoint(subarray.tel_positions.at(tel_id));
        auto tilted_tel_pos = tel_pos.transform_to_tilted(*tilted_frame);
        tiled_tel_positions.emplace(tel_id, Point2D(tilted_tel_pos.x(), tilted_tel_pos.y()));
    }
    return tiled_tel_positions;
}
std::tuple<double, double, double, double> HillasReconstructor::reconstruction_tilted_intersection()
{
    auto tel_pairs = get_tel_pairs();
    auto tilted_tel_positions = get_tiled_tel_position();
    std::vector<double> intersection_x;
    std::vector<double> intersection_y;
    std::vector<double> weight;
    for(const auto& [tel_id1, tel_id2]: tel_pairs)
    {
        auto hillas1 = nominal_hillas_dicts[tel_id1];
        auto hillas2 = nominal_hillas_dicts[tel_id2];
        auto line1 = Line2D({tilted_tel_positions.at(tel_id1), {cos(hillas1.psi), sin(hillas1.psi)}});
        auto line2 = Line2D({tilted_tel_positions.at(tel_id2), {cos(hillas2.psi), sin(hillas2.psi)}});
        auto intersection = line1.intersection(line2);
        if(intersection.has_value())
        {
            intersection_x.push_back(intersection->x());
            intersection_y.push_back(intersection->y());
            auto delta_1 = 1 - hillas1.width / hillas1.length;
            auto delta_2 = 1 - hillas2.width / hillas2.length;
            auto sin_part = sin(hillas1.psi - hillas2.psi);
            auto reduced_amplitude = hillas1.intensity * hillas1.intensity/ (hillas1.intensity + hillas2.intensity);
            weight.push_back(knonrad_weight(reduced_amplitude, delta_1, delta_2, sin_part));
        }
    }
    auto weight_map = Eigen::Map<Eigen::VectorXd>(weight.data(), weight.size());
    auto x_map = Eigen::Map<Eigen::VectorXd>(intersection_x.data(), intersection_x.size());
    auto y_map = Eigen::Map<Eigen::VectorXd>(intersection_y.data(), intersection_y.size());
    auto mean_x = x_map.dot(weight_map) / weight_map.sum(); 
    auto mean_y = y_map.dot(weight_map) / weight_map.sum();
    auto sigma_x = (x_map.array().square() * weight_map.array()).sum() / weight_map.sum() - mean_x * mean_x;
    auto sigma_y = (y_map.array().square() * weight_map.array()).sum() / weight_map.sum() - mean_y * mean_y;
    return std::make_tuple(mean_x, mean_y, sigma_x, sigma_y);
}
double HillasReconstructor::knonrad_weight(double reduced_amplitude, double delta_1, double delta_2, double sin_part)
{
    return reduced_amplitude * delta_1 * delta_2 * pow(sin_part, 2);
}

double HillasReconstructor::reconstruction_hmax(double altitude)
{
    
    Eigen::VectorXd hmax_v = Eigen::VectorXd::Zero(telescopes.size());
    Eigen::VectorXd weights = Eigen::VectorXd::Zero(telescopes.size());
    for(int i = 0; i < telescopes.size(); i++)
    {
        int tel_id = telescopes[i];
        auto hillas_r = nominal_hillas_dicts[tel_id].r;
        auto impact_parameter = impact_parameters[tel_id];
        auto hmax_estimate = impact_parameter/hillas_r;
        hmax_v(i) = hmax_estimate;
        weights(i) = nominal_hillas_dicts[tel_id].intensity;
    }
    double hmax = hmax_v.dot(weights) / weights.sum() * sin(altitude) + 4400;
    if(hmax > 100000)
    {
        hmax = 100000;
    }
    return hmax;
}