#include "R0Event.hh"
#include "spdlog/spdlog.h"


R0Camera::R0Camera(int n_pixels, int n_samples, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> high_gain_waveform, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> low_gain_waveform, Eigen::Vector<uint32_t, -1> high_gain_waveform_sum, Eigen::Vector<uint32_t, -1> low_gain_waveform_sum)
    : n_pixels(n_pixels), n_samples(n_samples)
{
    waveform[0] = std::move(high_gain_waveform);
    waveform[1] = std::move(low_gain_waveform);
    if(high_gain_waveform_sum.size() > 0) {
        waveform_sum = std::array<Eigen::Vector<uint32_t, -1>, 2>{std::move(high_gain_waveform_sum), std::move(low_gain_waveform_sum)};
    }
}

void R0Event::add_tel(int tel_id, int n_pixels, int n_samples, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> high_gain_waveform, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> low_gain_waveform, uint32_t* high_gain_waveform_sum, uint32_t* low_gain_waveform_sum)
{
    // Create waveform sum vectors
    Eigen::Vector<uint32_t, -1> high_gain_sum;
    Eigen::Vector<uint32_t, -1> low_gain_sum;
    if(high_gain_waveform_sum) {
        high_gain_sum = Eigen::Map<Eigen::Vector<uint32_t, -1>>(high_gain_waveform_sum, n_pixels);
        if(low_gain_waveform_sum) {
            low_gain_sum = Eigen::Map<Eigen::Vector<uint32_t, -1>>(low_gain_waveform_sum, n_pixels);
        }
    }

    // Add camera to container
    BaseTelContainer<R0Camera>::add_tel(tel_id, n_pixels, n_samples, std::move(high_gain_waveform), std::move(low_gain_waveform), std::move(high_gain_sum), std::move(low_gain_sum));
}