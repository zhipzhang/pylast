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

double PrototypeCalibrator::extract_waveform_base(const Eigen::VectorXf& waveform, int window_start, int window_size)
{
    return waveform.segment(window_start, window_size).mean();
}

float PrototypeCalibrator::integrate_waveform(const Eigen::VectorXf& waveform, int window_start, int window_end)
{
    return waveform.segment(window_start, window_end - window_start + 1).sum();
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
    const int start_index = 8;
    const int waveform_size = 240;

    // Normally the base_start is 0, except the peak is near the start of the waveform
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
            int base_start = 0;
            const auto& low_gain_waveform = c0_camera->low_gain_waveform.row(ipix);
            const auto& high_gain_waveform = c0_camera->high_gain_waveform.row(ipix);
            const auto& low_gain_waveform_filtered = median_filter(low_gain_waveform.segment(start_index, waveform_size), 3);
            const auto& high_gain_waveform_filtered = median_filter(high_gain_waveform.segment(start_index, waveform_size), 3);

            const auto& low_gain_waveform_sliding_average = sliding_average(Eigen::Map<const Eigen::VectorXf>(low_gain_waveform_filtered.data(), low_gain_waveform_filtered.size()), 6);     
            const auto& high_gain_waveform_sliding_average = sliding_average(Eigen::Map<const Eigen::VectorXf>(high_gain_waveform_filtered.data(), high_gain_waveform_filtered.size()), 6);


            auto [low_gain_peak_value, low_gain_peak_index] = extract_waveform_peak(low_gain_waveform_sliding_average);
            auto [high_gain_peak_value, high_gain_peak_index] = extract_waveform_peak(high_gain_waveform_sliding_average);

            if (low_gain_peak_index < baseline_interval_window)
            {
                // Let's move it to 15
                base_start = 15;
            }
            low_gain_base(ipix) = extract_waveform_base(low_gain_waveform_sliding_average, base_start, baseline_interval_window);
            high_gain_base(ipix) = extract_waveform_base(high_gain_waveform_sliding_average, base_start, baseline_interval_window);

            low_gain_peak(ipix) = low_gain_peak_value;
            high_gain_peak(ipix) = high_gain_peak_value;

            
            int sum_start = std::max(0, low_gain_peak_index - sum_before_peak_window);
            int sum_end = std::min(39, low_gain_peak_index + sum_after_peak_window);
            low_gain_area(ipix) = integrate_waveform(low_gain_waveform_sliding_average, sum_start, sum_end) - low_gain_base(ipix) * (sum_end - sum_start + 1);
            sum_start = std::max(0, high_gain_peak_index - sum_before_peak_window);
            sum_end = std::min(39, high_gain_peak_index + sum_after_peak_window);
            high_gain_area(ipix) = integrate_waveform(high_gain_waveform_sliding_average, sum_start, sum_end) - high_gain_base(ipix) * (sum_end - sum_start + 1);
            low_gain_peak_time(ipix) = low_gain_peak_index;
            high_gain_peak_time(ipix) = high_gain_peak_index ;

        }
        event.c1->add_tel_replace(tel_id, C1Camera{.n_pixels = c0_camera->n_pixels, .low_gain_base = std::move(low_gain_base), .high_gain_base = std::move(high_gain_base), .low_gain_peak = std::move(low_gain_peak), .high_gain_peak = std::move(high_gain_peak), .low_gain_area = std::move(low_gain_area), .high_gain_area = std::move(high_gain_area), .low_gain_peak_time = std::move(low_gain_peak_time), .high_gain_peak_time = std::move(high_gain_peak_time)});
    }
}

void PrototypeCalibrator::advanced_process(ArrayEvent& event)
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
    const int start_index = 8;
    const int waveform_size = 240;

    // Normally the base_start is 0, except the peak is near the start of the waveform
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
            int base_start = 0;
            const auto& low_gain_waveform = c0_camera->low_gain_waveform.row(ipix);
            const auto& high_gain_waveform = c0_camera->high_gain_waveform.row(ipix);
            const auto& low_gain_waveform_filtered = median_filter(low_gain_waveform.segment(start_index, waveform_size), 3);
            const auto& high_gain_waveform_filtered = median_filter(high_gain_waveform.segment(start_index, waveform_size), 3);

            const auto& low_gain_waveform_savitzky_golay = savitzky_golay_smoothing(low_gain_waveform_filtered, 31, 2);
            const auto& high_gain_waveform_savitzky_golay = savitzky_golay_smoothing(high_gain_waveform_filtered, 31, 2);

            const auto& low_gain_waveform_sliding_average = sliding_average(Eigen::Map<const Eigen::VectorXf>(low_gain_waveform_savitzky_golay.data(), low_gain_waveform_savitzky_golay.size()), 4);
            const auto& high_gain_waveform_sliding_average = sliding_average(Eigen::Map<const Eigen::VectorXf>(high_gain_waveform_savitzky_golay.data(), high_gain_waveform_savitzky_golay.size()), 4);


            auto [low_gain_peak_value, low_gain_peak_index] = extract_waveform_peak(low_gain_waveform_sliding_average);
            auto [high_gain_peak_value, high_gain_peak_index] = extract_waveform_peak(high_gain_waveform_sliding_average);

            if (low_gain_peak_index < baseline_interval_window)
            {
                // Let's move it to 15
                base_start = 24;
            }
            low_gain_base(ipix) = extract_waveform_base(low_gain_waveform_sliding_average, base_start, baseline_interval_window);
            high_gain_base(ipix) = extract_waveform_base(high_gain_waveform_sliding_average, base_start, baseline_interval_window);

            low_gain_peak(ipix) = low_gain_peak_value;
            high_gain_peak(ipix) = high_gain_peak_value;

            
            int sum_start = std::max(0, low_gain_peak_index - sum_before_peak_window);
            int sum_end = std::min(59, low_gain_peak_index + sum_after_peak_window);
            low_gain_area(ipix) = integrate_waveform(low_gain_waveform_sliding_average, sum_start, sum_end) - low_gain_base(ipix) * (sum_end - sum_start + 1);
            sum_start = std::max(0, high_gain_peak_index - sum_before_peak_window);
            sum_end = std::min(59, high_gain_peak_index + sum_after_peak_window);
            high_gain_area(ipix) = integrate_waveform(high_gain_waveform_sliding_average, sum_start, sum_end) - high_gain_base(ipix) * (sum_end - sum_start + 1);
            low_gain_peak_time(ipix) = low_gain_peak_index;
            high_gain_peak_time(ipix) = high_gain_peak_index ;

        }
        event.c1->add_tel_replace(tel_id, C1Camera{.n_pixels = c0_camera->n_pixels, .low_gain_base = std::move(low_gain_base), .high_gain_base = std::move(high_gain_base), .low_gain_peak = std::move(low_gain_peak), .high_gain_peak = std::move(high_gain_peak), .low_gain_area = std::move(low_gain_area), .high_gain_area = std::move(high_gain_area), .low_gain_peak_time = std::move(low_gain_peak_time), .high_gain_peak_time = std::move(high_gain_peak_time)});
    }
}

Eigen::VectorXf PrototypeCalibrator::median_filter(const Eigen::VectorXf& waveform, int kernel_size)
{
        if (kernel_size <= 0 || kernel_size % 2 == 0) {
            throw std::invalid_argument("kernel_size must be a positive odd number");
        }
    
        int n = waveform.size();
        int r = kernel_size / 2;
        Eigen::VectorXf y(n);
    
        std::vector<float> window;
        window.reserve(kernel_size);
    
        for (int i = 0; i < n; ++i) {
            window.clear();
            for (int j = -r; j <= r; ++j) {
                window.push_back(waveform(std::clamp(i + j, 0, n - 1)));
            }
            auto mid = window.begin() + r;
            std::nth_element(window.begin(), mid, window.end());
            y(i) = *mid;
        }
    
        return y;

}

Eigen::VectorXf PrototypeCalibrator::sliding_average(const Eigen::Map<const Eigen::VectorXf>& waveform, int window_size)
{
    int n  = waveform.size();
    if (n % window_size != 0)
    {
        throw std::invalid_argument("waveform size must be divisible by window size");
    }
    Eigen::VectorXf y(n / window_size);
    for(int i = 0; i < n / window_size; i++)
    {
        y(i) = waveform.segment(i * window_size, window_size).mean();
    }
    return y;
}

Eigen::VectorXf PrototypeCalibrator::savitzky_golay_smoothing(const Eigen::VectorXf& waveform, int window_size, int order)
{
    if (window_size < 0 || order < 0 || order >= window_size)
    {
        throw std::invalid_argument("invalid window size or order");
    }
    if (window_size % 2 == 0)
    {
        throw std::invalid_argument("window size must be odd");
    }
    Eigen::MatrixXf A(window_size, order + 1);
    int half_window = window_size / 2;
    for (int i = -half_window; i <= half_window; i++)
    {
        for (int j = 0; j <= order; j++)
        {
            A(i + half_window, j) = std::pow(i, j);
        }
    }

    Eigen::VectorXf res(waveform.size());
    // Compute the pseudo-inverse using Eigen's completeOrthogonalDecomposition
    Eigen::MatrixXf A_pinv = A.completeOrthogonalDecomposition().pseudoInverse();
    const Eigen::VectorXf& A_pinv_row0 = A_pinv.row(0);

    // First process the place where full window is contained

    for(int i = half_window; i < waveform.size() - half_window; i++)
    {
        res(i) = A_pinv_row0.dot(waveform.segment(i - half_window, window_size));
    }

    // Handle the beginning and end of the waveform
    Eigen::VectorXf begin_coefficients = A_pinv * waveform.segment(0, window_size);
    Eigen::VectorXf end_coefficients = A_pinv * waveform.segment(waveform.size() - window_size, window_size);
    for(int i = 0; i < half_window; i++)
    {
        int index = i - half_window;
        float results = 0;
        for (int j = 0; j <= order; j++)
        {
             results += begin_coefficients(j) * std::pow(index, j);
        }
        res(i) = results;
    }
    for(int i = waveform.size() - half_window; i < waveform.size(); i++)
    {
        int index = i - waveform.size() + half_window + 1;
        float results = 0;
        for (int j = 0; j <= order; j++)
        {
            results += end_coefficients(j) * std::pow(index, j);
        }
        res(i) = results;
    }
    return res;
}