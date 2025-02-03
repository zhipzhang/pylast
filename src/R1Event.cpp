#include "R1Event.hh"

R1Camera::R1Camera(int n_pixels, int n_samples, Eigen::Matrix<double, -1, -1, Eigen::RowMajor> waveform, Eigen::VectorXi gain_selection)
{
    this->n_pixels = n_pixels;
    this->n_samples = n_samples;
    this->waveform = std::move(waveform);
    this->gain_selection = std::move(gain_selection);
}