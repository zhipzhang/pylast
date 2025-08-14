#include "SubarrayDescription.hh"
#include "spdlog/spdlog.h"

const string TelescopeDescription::print() const
{
    return fmt::format("TelescopeDescription:\n  tel_name: {}\n  camera: {}\n  optics: {}\n", 
        tel_name,
        camera_description.print(),
        optics_description.print());
}

void SubarrayDescription::add_telescope(const telescope_id_t tel_id, TelescopeDescription &&tel_description, const std::array<double, 3> &tel_position)
{
    tels.emplace(tel_id, std::move(tel_description));
    tel_positions[tel_id] = tel_position;
}

std::vector<telescope_id_t> SubarrayDescription::get_ordered_telescope_ids() const
{
    std::vector<telescope_id_t> ordered_tel_ids;
    for(const auto& [tel_id, _] : tels)
    {
        ordered_tel_ids.push_back(tel_id);
    }
    std::sort(ordered_tel_ids.begin(), ordered_tel_ids.end());
    return ordered_tel_ids;
}
const string SubarrayDescription::print() const
{
    return fmt::format("SubarrayDescription:\n  tel_descriptions: <dict[tel_id, TelescopeDescription]>\n  tel_positions: <dict[tel_id, array<double,3>]>\n  reference_position: <array<double,3>>\n");
}