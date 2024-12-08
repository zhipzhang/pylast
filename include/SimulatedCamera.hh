/**
 * @file SimulatedCamera.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  class to describe a simulated camera image
 * @version 0.1
 * @date 2024-12-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */
 #pragma once

class ImageParameters;
#include "Eigen/Dense"
#include "TelImpactParameter.hh"
class SimulatedCamera {
public:
    SimulatedCamera() = default;
    ~SimulatedCamera() = default;
    SimulatedCamera(const SimulatedCamera&) = default;
    SimulatedCamera(SimulatedCamera&&) = default;
    SimulatedCamera& operator=(const SimulatedCamera&) noexcept = default;
    SimulatedCamera& operator=(SimulatedCamera&&) noexcept = default;
    SimulatedCamera(int n_pixels, int* pe_count, double impact_parameter);
    int true_image_sum;
    Eigen::VectorXi true_image;
    TelImpactParameter impact_parameter;
    //ImageParameters true_image;

 };