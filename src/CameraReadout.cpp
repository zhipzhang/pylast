#include "CameraReadout.hh"
#include "spdlog/fmt/fmt.h"

CameraReadout::CameraReadout(string camera_name, double sampling_rate, double reference_pulse_sample_width, int n_channels, int n_pixels, int n_samples, double* reference_pulse_shape, int n_ref_shape, int l_ref_shape):
    camera_name(camera_name), sampling_rate(sampling_rate), reference_pulse_sample_width(reference_pulse_sample_width), n_channels(n_channels), n_pixels(n_pixels), n_samples(n_samples)
{
    this->reference_pulse_shape = Eigen::MatrixXd(Eigen::Map<Eigen::MatrixXd, Eigen::RowMajor>(reference_pulse_shape, n_ref_shape, l_ref_shape));
}
const string CameraReadout::print() const
{
    return fmt::format("CameraReadout(\n"
    "    camera_name: {}\n"
    "    sampling_rate: {:.3f} Hz\n"
    "    reference_pulse_sample_width: {:.3f} ns\n"
    "    n_channels: {}\n"
    "    n_pixels: {}\n"
    "    n_samples: {}\n"
    ")", camera_name, sampling_rate, reference_pulse_sample_width, n_channels, n_pixels, n_samples);
}