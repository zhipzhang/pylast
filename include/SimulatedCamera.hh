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
#include "spdlog/fmt/fmt.h"

class SimulatedCamera {
public:
    SimulatedCamera() = default;
    SimulatedCamera(int n_pixels, int* pe_count, double impact_parameter);

    /**
     * @brief Sum intensity of the true image [p.e.]
     * 
     */
    int true_image_sum;
    /**
     * @brief True image per pixel [p.e.]
     * 
     */
    Eigen::VectorXi true_image;
    /**
     * @brief True Impact Parameter object.
     * 
     */
    TelImpactParameter impact;
    //ImageParameters true_image;
    std::string print() const {
        return fmt::format("SimulatedCamera:\n"
                         "\ttrue_image_sum: {}\n"
                         "\ttrue_image: array of {} pixels\n"
                         "\timpact_parameter: ({:.2f} ± {:.2f})",
                         true_image_sum,
                         true_image.size(),
                         impact.distance,
                         impact.distance_error);
    }

 };