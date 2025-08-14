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
#include "ImageParameters.hh"

class SimulatedCamera {
public:

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
    double impact_parameter;
    /**
     * @brief Fake image with poisson noise added.
     * 
     */
    Eigen::VectorXd fake_image;
    Eigen::Vector<bool, -1> fake_image_mask; // Mask for the fake image
    Eigen::VectorXd pe_amplitude; // Amplitude of the photoelectrons
    Eigen::VectorXd pe_time; // Time of the photoelectrons
    double time_range_10_90;

    ImageParameters fake_image_parameters;
    //ImageParameters true_image;
    std::string print() const {
        return fmt::format("SimulatedCamera:\n"
                         "\ttrue_image_sum: {}\n"
                         "\ttrue_image: array of {} pixels\n"
                         "\timpact_parameter: ({:.2f})",
                         true_image_sum,
                         true_image.size(),
                         impact_parameter
                         );
    }

 };