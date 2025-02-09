/**
 * @file ImageExtractor.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2025-01-31
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include "Eigen/Dense"
#include <cstddef>
#include <utility>
#include "Eigen/src/Core/Matrix.h"
#include "SubarrayDescription.hh"
#include "optional"
/**
 * @brief Extract the waveform around the peak
 * 
 * @param waveform  (n_pixels, n_samples)
 * @param peak_index  (n_pixels)
 * @param window_width  (n_pixels)
 * @param window_shift  (n_pixels)
 * @param sampling_rate_ghz  double
 * @return std::pair<Eigen::VectorXd, Eigen::VectorXd> 0: charge, 1: time
 */
std::pair<Eigen::VectorXd, Eigen::VectorXd>  extract_around_peak(const Eigen::Matrix<double, -1, -1, Eigen::RowMajor>& waveform, const Eigen::VectorXi& peak_index, const Eigen::VectorXi& window_width, const Eigen::VectorXi& window_shift, double sampling_rate_ghz);


class ImageExtractor
{
    public:
       ImageExtractor(SubarrayDescription& subarray);
       virtual ~ImageExtractor() = default;

       virtual std::pair<Eigen::VectorXd, Eigen::VectorXd> operator()(const Eigen::Matrix<double, -1, -1, Eigen::RowMajor>& waveform, const Eigen::VectorXi& gain_selection, int tel_id) = 0;
       Eigen::VectorXi get_peak_index(const Eigen::Matrix<double, -1, -1, Eigen::RowMajor>& waveform);
       Eigen::VectorXd compute_integration_correction(const Eigen::MatrixXd& reference_pulse, double reference_pulse_sample_width_ns, double sample_width_ns, int window_width, int window_shift);
       Eigen::VectorXd get_cached_integration_correction() const { return *cached_correction;}
    protected:
        SubarrayDescription& subarray;
        std::unordered_map<int, double> sampling_rate_ghz;
        std::optional<Eigen::VectorXd> cached_correction;


};

class FullWaveFormExtractor: public ImageExtractor
{
    public:
    FullWaveFormExtractor(SubarrayDescription& subarray);
    virtual ~FullWaveFormExtractor() = default;

    std::pair<Eigen::VectorXd, Eigen::VectorXd> operator()(const Eigen::Matrix<double, -1, -1, Eigen::RowMajor>& waveform, const Eigen::VectorXi& gain_selection, int tel_id) override
    {
        int window_width = waveform.cols();
        double sampling_rate_ghz = this->sampling_rate_ghz[tel_id];
        return extract_around_peak(waveform, Eigen::VectorXi::Zero(waveform.rows()), Eigen::VectorXi::Constant(waveform.rows(), window_width), Eigen::VectorXi::Zero(waveform.rows()), sampling_rate_ghz);
    }
};

class LocalPeakExtractor: public ImageExtractor
{
    public:
    LocalPeakExtractor(SubarrayDescription& subarray, int window_width = 7, int window_shift = 3, bool apply_correction = true);
    virtual ~LocalPeakExtractor() = default;

    std::pair<Eigen::VectorXd, Eigen::VectorXd> operator()(const Eigen::Matrix<double, -1, -1, Eigen::RowMajor>& waveform, const Eigen::VectorXi& gain_selection, int tel_id) override
    {
        auto peak_index = this->get_peak_index(waveform);
        double sampling_rate_ghz = this->sampling_rate_ghz[tel_id];
        auto [charge, peak_time] = extract_around_peak(waveform, peak_index, Eigen::VectorXi::Constant(waveform.rows(), this->window_width), Eigen::VectorXi::Constant(waveform.rows(), this->window_shift), sampling_rate_ghz);
        auto readout = subarray.tels[tel_id].camera_description.camera_readout;
        if (this->apply_correction)
        {
            this->correction(charge, gain_selection, readout, sampling_rate_ghz);
        }
        return std::make_pair(charge, peak_time);
    }

    private:
    void correction(Eigen::VectorXd& charge, const Eigen::VectorXi& gain_selection, const CameraReadout& readout, double sampling_rate_ghz);
    int window_width;
    int window_shift;
    bool apply_correction;
};
