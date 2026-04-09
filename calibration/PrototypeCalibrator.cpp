#include "PrototypeCalibrator.hh"
#include "DL0Event.hh"
#include "Eigen/src/Core/Matrix.h"
#include "spdlog/spdlog.h"


void PrototypeCalibrator::registerParams()
{
    registerParam<int>("baseline_interval_window", 50, baseline_interval_window);
    registerParam<int>("sum_before_peak_window", 10, sum_before_peak_window);
    registerParam<int>("sum_after_peak_window", 30, sum_after_peak_window);
}

double PrototypeCalibrator::extract_waveform_base(const Eigen::VectorXf& waveform, int window)
{
    return waveform.segment(0, window).mean();
}

float PrototypeCalibrator::integrate_waveform(const Eigen::VectorXf& waveform, int window_start, int window_end)
{
    return waveform.segment(window_start, window_end - window_start).sum();
}

std::pair<float, int> PrototypeCalibrator::extract_waveform_peak(const Eigen::VectorXf& waveform)
{
    Eigen::MatrixXf::Index maxIndex;
    waveform.maxCoeff(&maxIndex);
    return std::make_pair(static_cast<float>(waveform(maxIndex)), maxIndex);
}

void PrototypeCalibrator::operator()(ArrayEvent& event)
{
    if(!event.c0.has_value())
    {
        spdlog::warn("C0Event is not found in the event");
        return;
    }
    if(!event.c1.has_value())
    {
        event.c1 = C1Event();
    }
    for(const auto& [tel_id, c0_camera]: event.c0->tels)
    {
        Eigen::VectorXf low_gain_base{c0_camera->n_pixels};
        Eigen::VectorXf high_gain_base{c0_camera->n_pixels};
        Eigen::VectorXf low_gain_peak{c0_camera->n_pixels};
        Eigen::VectorXf high_gain_peak{c0_camera->n_pixels};
        Eigen::VectorXf low_gain_area{c0_camera->n_pixels};
        Eigen::VectorXf high_gain_area{c0_camera->n_pixels};
        Eigen::VectorXi low_gain_peak_time{c0_camera->n_pixels};
        Eigen::VectorXi high_gain_peak_time{c0_camera->n_pixels};
        for(int ipix = 0; ipix < c0_camera->n_pixels; ipix++)
        {
            low_gain_base(ipix) = extract_waveform_base(c0_camera->low_gain_waveform.row(ipix), baseline_interval_window);
            high_gain_base(ipix) = extract_waveform_base(c0_camera->high_gain_waveform.row(ipix), baseline_interval_window);
            auto [low_gain_peak_value, low_gain_peak_index] = extract_waveform_peak(c0_camera->low_gain_waveform.row(ipix));
            low_gain_peak(ipix) = low_gain_peak_value;
            auto [high_gain_peak_value, high_gain_peak_index] = extract_waveform_peak(c0_camera->high_gain_waveform.row(ipix));
            high_gain_peak(ipix) = high_gain_peak_value;
            low_gain_area(ipix) = integrate_waveform(c0_camera->low_gain_waveform.row(ipix), low_gain_peak_index - sum_before_peak_window, low_gain_peak_index + sum_after_peak_window) - low_gain_base(ipix) * (sum_before_peak_window + sum_after_peak_window);
            high_gain_area(ipix) = integrate_waveform(c0_camera->high_gain_waveform.row(ipix), high_gain_peak_index - sum_before_peak_window, high_gain_peak_index + sum_after_peak_window) - high_gain_base(ipix) * (sum_before_peak_window + sum_after_peak_window);
            low_gain_peak_time(ipix) = low_gain_peak_index;
            high_gain_peak_time(ipix) = high_gain_peak_index ;

        }
        event.c1->add_tel(tel_id, C1Camera{.n_pixels = c0_camera->n_pixels, .low_gain_base = std::move(low_gain_base), .high_gain_base = std::move(high_gain_base), .low_gain_peak = std::move(low_gain_peak), .high_gain_peak = std::move(high_gain_peak), .low_gain_area = std::move(low_gain_area), .high_gain_area = std::move(high_gain_area), .low_gain_peak_time = std::move(low_gain_peak_time), .high_gain_peak_time = std::move(high_gain_peak_time)});
    }
    if(!event.dl0.has_value())
    {
        event.dl0 = DL0Event();
        for(const auto& [tel_id, c1_camera]: event.c1->tels)
        {
            DL0Camera dl0camera;
            dl0camera.image = Eigen::VectorXd::Zero(c1_camera->n_pixels);
            dl0camera.peak_time = Eigen::VectorXd::Zero(c1_camera->n_pixels);
            for(int ipix = 0; ipix < c1_camera->n_pixels; ipix++)
            {
                dl0camera.image(ipix) = c1_camera->low_gain_area(ipix) / 10;
            }
            for(int ipix = 0; ipix < c1_camera->n_pixels; ipix++)
            {
                dl0camera.peak_time(ipix) = c1_camera->low_gain_peak_time(ipix);
            }
            event.dl0->add_tel(tel_id, std::move(dl0camera));
        }
    }

}