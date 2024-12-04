/**
 * @file CameraDescription.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Description of the camera
 * @version 0.1
 * @date 2024-11-28
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include <string>
#include "CameraGeometry.hh"
#include "CameraReadout.hh"
using std::string;

class CameraDescription
{
public:
    /** @brief Name of the camera */
    string camera_name;
    /** @brief Geometry of the camera */
    CameraGeometry camera_geometry;
    /** @brief Readout of the camera */
    CameraReadout camera_readout;
    
    CameraDescription() = default;
    // Use pass by value and move constructor
    CameraDescription(string camera_name, CameraGeometry camera_geometry, CameraReadout camera_readout);

    CameraDescription(CameraDescription&& other) noexcept = default;
    CameraDescription(const CameraDescription& other) = default;
    CameraDescription& operator=(CameraDescription&& other) noexcept = default;
    CameraDescription& operator=(const CameraDescription& other) = default;
    ~CameraDescription() = default;
    const string print() const;
};
