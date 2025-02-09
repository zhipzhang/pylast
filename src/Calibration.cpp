#include "Calibration.hh"

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