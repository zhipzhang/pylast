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

#include <optional>
class HillasParameter
{
    public:
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
    double pixels_width_1;
    double pixels_width_2;
    double intensity_width_1;
    double intensity_width_2;
};
class ConcentrationParameter
{
    public:
    double concentration_cog; // one pixel diameter from the cog
    double concentration_core; // all_pixels inside the hillas ellipse, transformed to the hillas ellipse
    double concentration_pixel; // brightest pixel
};

class MorphologyParameter
{
    public:
    int n_pixels;
    int n_islands;
    int n_small_islands;
    int n_medium_islands;
    int n_large_islands;
    
};

class IntensityParameter
{
    public:
    double intensity_max;
    double intensity_mean;
    double intensity_std;
    double intensity_skewness;
    double intensity_kurtosis;
};
class ExtraParameters
{
    public:
    double miss = 0;
    double disp = 0;
    double theta = 0;
    double true_psi = 0;
    double cog_err = 0;
    double beta_err = 0;
};
class ImageParameters {
public:
    HillasParameter hillas;
    LeakageParameter leakage;
    ConcentrationParameter concentration;
    MorphologyParameter morphology;
    IntensityParameter intensity;
    ExtraParameters extra;
};
