#include "TelMonitor.hh"
#include "Eigen/Dense"

TelMonitor::TelMonitor(int n_channels, int n_pixels, Eigen::Matrix<double, -1, -1, Eigen::RowMajor> pedestal_per_sample, Eigen::Matrix<double, -1, -1, Eigen::RowMajor> dc_to_pe)
{
    this->n_channels = n_channels;
    this->n_pixels = n_pixels;
    this->pedestal_per_sample = std::move(pedestal_per_sample);
    this->dc_to_pe = std::move(dc_to_pe);
}