/**
 * @file Calibration.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  Some Useful Functions for Calibration
 * @version 0.1
 * @date 2025-01-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "Eigen/Dense"
#include "SubarrayDescription.hh"
#include "ArrayEvent.hh"
#include "ImageExtractor.hh"

 /**
  * @brief Select the gain channel by threshold
  * 
  * @param waveform  2-channel waveform (assume the first channel is the high gain channel, sometimes the second channel is 0)
  * @param threshold threshold for the low gain channel
  * @return Eigen::VectorXi
  */
 Eigen::VectorXi select_gain_channel_by_threshold(const std::array<Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>, 2>& waveform, const double threshold);


class Calibrator
{
    public:
    template<typename... Args>
    Calibrator(SubarrayDescription& subarray, const std::string& image_extractor_type = "FullWaveFormExtractor", Args&&... args);
    ~Calibrator() = default;

    void operator()(ArrayEvent& event);
    std::unique_ptr<ImageExtractor> image_extractor;
    private:
        SubarrayDescription& subarray;
};

class ImageExtractorFactory
{
    public:
    template<typename Extractor, typename... Args>
    static std::unique_ptr<Extractor> create(SubarrayDescription& subarray, Args&&... args)
    {
        return std::make_unique<Extractor>(subarray, std::forward<Args>(args)...);
    }
};