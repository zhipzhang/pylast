#include "CameraReadout.hh"
#include "spdlog/fmt/fmt.h"

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
