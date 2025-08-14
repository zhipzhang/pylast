

#include "CameraDescription.hh"
#include "spdlog/fmt/fmt.h"

const string CameraDescription::print() const
{
    return fmt::format("CameraDescription(\n"
    "    camera_name: {}\n"
    "    camera_geometry: {}\n"
    "    camera_readout: {}\n"
    ")", camera_name, camera_geometry.print(), camera_readout.print());
}
