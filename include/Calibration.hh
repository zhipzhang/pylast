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

#include "SubarrayDescription.hh"
#include "ArrayEvent.hh"
#include "ImageExtractor.hh"
#include "Configurable.hh"
 /**
  * @brief Select the gain channel by threshold
  * 
  * @param waveform  2-channel waveform (assume the first channel is the high gain channel, sometimes the second channel is 0)
  * @param threshold threshold for the low gain channel
  * @return Eigen::VectorXi
  */
 Eigen::VectorXi select_gain_channel_by_threshold(const std::array<Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>, 2>& waveform, const double threshold);


class ImageExtractorFactory
{
    public:
    ImageExtractorFactory() = default;
    template<typename Extractor, typename... Args>
    static std::unique_ptr<ImageExtractor> create(const SubarrayDescription& subarray, Args&&... args)
    {
        return std::make_unique<Extractor>(subarray, std::forward<Args>(args)...);
    }
    static ImageExtractorFactory& get_instance()
    {
        static ImageExtractorFactory instance;
        return instance;
    }
    ImageExtractorFactory(ImageExtractorFactory const&) = delete;
    void operator=(ImageExtractorFactory const&) = delete;
    ~ImageExtractorFactory() = default;
};

class Calibrator: public Configurable
{
    public:
    DECLARE_CONFIGURABLE_DEFINITIONS(const SubarrayDescription&, subarray, Calibrator);
    static json get_default_config();
    json default_config() const override {return get_default_config();}
    void configure(const json& config) override;
    ~Calibrator() = default;

    void operator()(ArrayEvent& event);
    std::unique_ptr<ImageExtractor> image_extractor;
    private:
        const SubarrayDescription& subarray;
        std::string image_extractor_type;
};