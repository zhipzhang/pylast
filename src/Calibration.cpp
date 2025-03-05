#include "Calibration.hh"
#include "Configurable.hh"
#include <stdexcept>

Eigen::VectorXi select_gain_channel_by_threshold(const std::array<Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>, 2>& waveform, const double threshold)
{
    // Matrix is (n_pixels, n_samples)
    // Vector returned is (n_pixels)
    if(waveform[1].isZero())
    {
        return Eigen::VectorXi::Zero(waveform[0].rows());
    }
    else 
    {
        // If the high gain channel exceeds the threshold, select the low gain channel
        Eigen::VectorXi gain_selector = Eigen::VectorXi::Zero(waveform[0].rows());
        for(int i = 0; i < waveform[0].rows(); i++)
        {
            if((waveform[0].row(i).array() > threshold).any())
            {
                gain_selector(i) = 1;
            }
        }
        return gain_selector;
    }
}

json Calibrator::get_default_config()
{
    std::string default_config = R"(
    {
        "image_extractor_type": "LocalPeakExtractor"
    }
    )";
    json base_config = Configurable::from_string(default_config);
    base_config["LocalPeakExtractor"] = LocalPeakExtractor::get_default_config();
    return base_config;
}

void Calibrator::configure(const json &config)
{
    try {
        const json& cfg = config.contains("Calibrator") ? config.at("Calibrator") : config;
        image_extractor_type = cfg["image_extractor_type"];
        if(image_extractor_type == "LocalPeakExtractor")
        {
            image_extractor = ImageExtractorFactory::create<LocalPeakExtractor>(subarray, cfg);
        }
        else if(image_extractor_type == "FullWaveFormExtractor")
        {
            image_extractor = ImageExtractorFactory::create<FullWaveFormExtractor>(subarray);
        }
        else {
            throw std::runtime_error("Unknow ImageExtractor type: " + image_extractor_type);
        }
    }
    catch(const std::exception& e) {
        throw std::runtime_error("Error configuring Calibrator: " + std::string(e.what()));
    }
}
void Calibrator::operator()(ArrayEvent& event)
{
    if(!event.dl0)
    {
        event.dl0 = DL0Event();
    }
    for(const auto& [tel_id, r1_camera]: event.r1->tels)
    {
        auto [charge, peak_time] = (*image_extractor)(r1_camera->waveform, r1_camera->gain_selection, tel_id);
        event.dl0->add_tel(tel_id, charge, peak_time);
    }
}