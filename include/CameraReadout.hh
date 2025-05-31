/**
 * @file CameraReadout.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Camera readout information
 * @version 0.1
 * @date 2024-12-02
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include <string>
#include "Eigen/Dense"
using std::string;
using Eigen::MatrixXd;
using Eigen::VectorXd;

class CameraReadout
{
public:
    /** @brief Name of the camera */
    string camera_name;
    /** @brief Sampling rate of the waveform [Hz] */
    double sampling_rate;
    /** @brief Expected pulse shape for a signal in the waveform. 2 dimensional, first dimension is gain channel */
    Eigen::MatrixXd reference_pulse_shape;
    /** @brief The amount of time corresponding to each sample in reference_pulse_shape [ns] */
    double reference_pulse_sample_width;
    /** @brief Number of gain channels */
    int n_channels;
    /** @brief Number of pixels */
    int n_pixels;
    /** @brief Number of waveform samples for normal events */
    int n_samples;

    CameraReadout() = default;
    CameraReadout(string camera_name, double sampling_rate, double reference_pulse_sample_width, int n_channels, int n_pixels, int n_samples, Eigen::MatrixXd reference_pulse_shape);

    const string print() const;
};