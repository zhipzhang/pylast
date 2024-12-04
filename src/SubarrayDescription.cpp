#include "SubarrayDescription.hh"
#include "spdlog/spdlog.h"
TelescopeDescription::TelescopeDescription(CameraDescription camera_description, OpticsDescription optics_description):
    camera_description(std::move(camera_description)), optics_description(std::move(optics_description))
{
    spdlog::debug("TelescopeDescription created with camera_description");
}

void SubarrayDescription::add_telescope(const telescope_id_t tel_id, TelescopeDescription &&tel_description, const std::array<double, 3> &tel_position)
{
    tel_descriptions.emplace(tel_id, std::move(tel_description));
    tel_positions[tel_id] = tel_position;
}