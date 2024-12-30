#include "R0Event.hh"


void R0Camera::initialize_waveform(int n_pixels, int n_samples) {
    n_pixels = n_pixels;
    n_samples = n_samples;
    for(auto& w : waveform) {
        w.resize(n_pixels, n_samples);
    }
}
void R0Camera::initialize_waveform_sum(int n_pixels) {
    for(auto& w : waveform_sum) {
        w.resize(n_pixels);
    }
}
void R0Camera::set_waveform(int n_pixels, int n_samples,  Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>&& high_gain, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>&& low_gain) {
    initialize_waveform(n_pixels, n_samples);
    waveform[0] = std::move(high_gain);
    waveform[1] = std::move(low_gain);
}
void R0Camera::set_waveform_sum(int n_pixels, Eigen::Vector<uint32_t, -1>&& high_gain_sum, Eigen::Vector<uint32_t, -1>&& low_gain_sum) {
    initialize_waveform_sum(n_pixels);
    waveform_sum[0] = std::move(high_gain_sum);
    waveform_sum[1] = std::move(low_gain_sum);
}




