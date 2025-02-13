/**
 * @file ImageParameters.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  class to describe image parameters
 * @version 0.1
 * @date 2024-12-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */

 #pragma once

class HillasParameter
{
    public:
    HillasParameter() = default;
    ~HillasParameter() = default;
    double length;   // [rad]  
    double width;    // [rad]
    double psi;      // [rad] orientation of the major axis
    double x;        // [rad]
    double y;        // [rad]
    double skewness; // [ ]
    double kurtosis; // [ ]
    double intensity; // [p.e.]
    double r;        // [rad]  distance from the center of the camera
    double phi;      // [rad]  angle from the center of the camera

};
class LeakageParameter
{
    public:
    LeakageParameter() = default;
    ~LeakageParameter() = default;
    double pixels_width_1;
    double pixels_width_2;
    double intensity_width_1;
    double intensity_width_2;
};

class ImageParameters {
public:
    ImageParameters() = default;
    ~ImageParameters() = default;
    ImageParameters(const ImageParameters& other) = default;
    HillasParameter hillas;
    LeakageParameter leakage;


};
