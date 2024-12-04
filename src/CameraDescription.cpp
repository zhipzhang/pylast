

#include "CameraDescription.hh"
#include "spdlog/fmt/fmt.h"

CameraDescription::CameraDescription(string camera_name, CameraGeometry camera_geometry, CameraReadout camera_readout):
    camera_name(camera_name), camera_geometry(std::move(camera_geometry)), camera_readout(std::move(camera_readout))
{
}

const string CameraDescription::print() const
{
    return fmt::format("CameraDescription(\n"
    "    camera_name: {}\n"
    "    camera_geometry: {}\n"
    "    camera_readout: {}\n"
    ")", camera_name, camera_geometry.print(), camera_readout.print());
}
