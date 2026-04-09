#pragma once
#include "C1Event.hh"
#include "ArrayEvent.hh"
#include "ConfigMacros.hh"
#include "ConfigSystem.hh"
class PrototypeCalibrator: public config::Configurable
{
    public:
    CONFIG_CONSTRUCTORS(PrototypeCalibrator);
    void registerParams() override;
    void setUp() override;
    void operator()(ArrayEvent& event);

    double extract_waveform_base(const Eigen::VectorXf& waveform, int window);
    float integrate_waveform(const Eigen::VectorXf& waveform, int window_start, int window_end);

    std::pair<float, int> extract_waveform_peak(const Eigen::VectorXf& waveform);


    static constexpr int ALL_SAMPLES = 256;
    static constexpr int BASE_WINDOW = 50;

    private:
        int baseline_interval_window;
        int sum_before_peak_window;
        int sum_after_peak_window;
};

