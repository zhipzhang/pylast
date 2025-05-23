#include "R0Event.hh"
#include "spdlog/spdlog.h"
#include <cstdint>


R0Camera::R0Camera(int n_pixels, int n_samples, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> high_gain_waveform, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> low_gain_waveform, Eigen::Vector<uint32_t, -1> high_gain_waveform_sum, Eigen::Vector<uint32_t, -1> low_gain_waveform_sum)
    : n_pixels(n_pixels), n_samples(n_samples)
{
    waveform[0] = std::move(high_gain_waveform);
    waveform[1] = std::move(low_gain_waveform);
    if(high_gain_waveform_sum.size() > 0) {
        waveform_sum = std::array<Eigen::Vector<uint32_t, -1>, 2>{std::move(high_gain_waveform_sum), std::move(low_gain_waveform_sum)};
    }
}

void R0Event::add_tel(int tel_id, int n_pixels, int n_samples, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> high_gain_waveform, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> low_gain_waveform, uint32_t* high_gain_waveform_sum_ptr, uint32_t* low_gain_waveform_sum_ptr)
{
    // Create Eigen::Vector objects for waveform sums
    Eigen::Vector<uint32_t, -1> hg_sum_vector;
    Eigen::Vector<uint32_t, -1> lg_sum_vector;

    if(high_gain_waveform_sum_ptr) {
        hg_sum_vector = Eigen::Map<const Eigen::Vector<uint32_t, -1>>(high_gain_waveform_sum_ptr, n_pixels);
        // Only process low_gain_waveform_sum if high_gain_waveform_sum was also provided
        if(low_gain_waveform_sum_ptr) {
            lg_sum_vector = Eigen::Map<const Eigen::Vector<uint32_t, -1>>(low_gain_waveform_sum_ptr, n_pixels);
        }
    }
    // If high_gain_waveform_sum_ptr is null, hg_sum_vector and lg_sum_vector remain empty.
    // This is handled correctly by the R0Camera constructor.

    // Add camera to container, moving the data
    BaseTelContainer<R0Camera>::add_tel(tel_id, n_pixels, n_samples, std::move(high_gain_waveform), std::move(low_gain_waveform), std::move(hg_sum_vector), std::move(lg_sum_vector));
}