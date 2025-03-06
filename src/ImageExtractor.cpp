#include "ImageExtractor.hh"
#include <iostream>
ImageExtractor::ImageExtractor(const SubarrayDescription& subarray):
    subarray(subarray)
{
    for(auto [tel_id, tel_config]: subarray.tels)
    {
        sampling_rate_ghz[tel_id] = tel_config.camera_description.camera_readout.sampling_rate;
    }
}
// TODO: Fix this function 
Eigen::VectorXd ImageExtractor::compute_integration_correction(const Eigen::MatrixXd& reference_pulse, double reference_pulse_sample_width_ns, double sample_width_ns, int window_width, int window_shift)
{
    int n_channels = reference_pulse.rows();
    Eigen::VectorXd correction(n_channels);
    correction.setOnes(); // Initialize correction to 1 for all channels

    for (int ich = 0; ich < n_channels; ich++)
    {
        Eigen::VectorXd pulse = reference_pulse.row(ich);
        double max_pulse_time = (pulse.size() - 0.5) * reference_pulse_sample_width_ns;
        Eigen::VectorXd pulse_shape_x = Eigen::VectorXd::LinSpaced(pulse.size() , 0.5 * reference_pulse_sample_width_ns, max_pulse_time);
        
        // Create sampled_edges for histogram
        int n_sampled_edges = static_cast<int>(std::ceil(max_pulse_time / sample_width_ns)) + 1;
        Eigen::VectorXd sampled_edges = Eigen::VectorXd::LinSpaced(n_sampled_edges, 0, max_pulse_time);

        // Simulate histogram using manual binning
        Eigen::VectorXd sampled_pulse = Eigen::VectorXd::Zero(n_sampled_edges - 1);
        for (int i = 0; i < pulse.size(); ++i) {
            double time = pulse_shape_x(i);
            for (int j = 0; j < n_sampled_edges - 1; ++j) {
                if (time >= sampled_edges(j) && time < sampled_edges(j + 1)) {
                    sampled_pulse(j) += pulse(i);
                    break;
                }
            }
        }
        
        // Normalize to get density
        double total_weight = pulse.sum();
        for (int j = 0; j < n_sampled_edges - 1; ++j) {
            sampled_pulse(j)  = sampled_pulse(j)  / total_weight;
        }

        int n_samples = sampled_pulse.size();
        int start = 0;
        int max_index = 0;
        sampled_pulse.maxCoeff(&max_index);
        start = max_index - window_shift;
        if (start < 0) {
            start = 0;
        }
        int end = start + window_width;
        if (end > n_samples) {
            end = n_samples;
        }
        if (start >= end) {
            continue;
        }

        double integration = 0.0;
        for (int i = start; i < end; ++i) {
            integration += sampled_pulse(i) ;
        }

        if (integration != 0.0) {
            correction(ich) = 1.0 / integration;
        }
    }
    
    // Return the correction factors
    return correction;
}
FullWaveFormExtractor::FullWaveFormExtractor(const SubarrayDescription& subarray):
    ImageExtractor(subarray)
{
}

std::pair<Eigen::VectorXd, Eigen::VectorXd>  extract_around_peak(const Eigen::Matrix<double, -1, -1, Eigen::RowMajor>& waveform, const Eigen::VectorXi& peak_index, const Eigen::VectorXi& window_width, const Eigen::VectorXi& window_shift, double sampling_rate_ghz)
{
    auto window_start = peak_index.array() - window_shift.array();
    auto window_end = window_start.array() + window_width.array();
    int n_pixels = waveform.rows();
    Eigen::VectorXd charge = Eigen::VectorXd::Zero(n_pixels);
    Eigen::VectorXd peak_time = Eigen::VectorXd::Zero(n_pixels);

    // For each pixel, sum the waveform values within the window
    for(int ipix = 0; ipix < n_pixels; ipix++) {
        int start = std::max(0, window_start(ipix));
        int end = std::min((int)waveform.cols(), window_end(ipix));
        
        // Sum the waveform values in the window
        charge(ipix) = waveform.block(ipix, start, 1, end - start).sum();
        
        // Calculate weighted time average
        double time_sum = 0;
        for(int i = start; i < end; i++) {
            if(waveform(ipix, i) > 0) {
                time_sum += i * waveform(ipix, i);
            }
        }
        peak_time(ipix) = time_sum / charge(ipix) / sampling_rate_ghz;
    }

    return std::make_pair(charge, peak_time);
}

Eigen::VectorXi ImageExtractor::get_peak_index(const Eigen::Matrix<double, -1, -1, Eigen::RowMajor>& waveform)
{
    Eigen::VectorXi peak_index = Eigen::VectorXi::Zero(waveform.rows());
    for(int ipix = 0; ipix < waveform.rows(); ipix++) {
        Eigen::MatrixXd::Index maxIndex;
        waveform.row(ipix).maxCoeff(&maxIndex);
        peak_index(ipix) = maxIndex;
    }
    return peak_index;
}
json LocalPeakExtractor::get_default_config()
{
    std::string config = R"(
    {
        "window_width": 7,
        "window_shift": 3,
        "apply_correction": true
    }
    )";
    return Configurable::from_string(config);
}
void LocalPeakExtractor::configure(const json& config)
{
    try {
        const json& cfg = config.contains("LocalPeakExtractor") ? config.at("LocalPeakExtractor") : config;
        window_width = cfg["window_width"];
        window_shift = cfg["window_shift"];
        apply_correction = cfg["apply_correction"];
    }
    catch(const std::exception& e) {
        throw std::runtime_error("Error configuring LocalPeakExtractor: " + std::string(e.what()));
    }
}
void LocalPeakExtractor::correction(Eigen::VectorXd& charge, const Eigen::VectorXi& gain_selection, const CameraReadout& readout, double sampling_rate_ghz)
{
    if(!this->cached_correction.has_value())
    {
        Eigen::VectorXd correction = this->compute_integration_correction(
            readout.reference_pulse_shape,
            readout.reference_pulse_sample_width,
            1/sampling_rate_ghz,
            this->window_width,
            this->window_shift
        );
        this->cached_correction = std::move(correction);
    }
    
    for(int ipix = 0; ipix < charge.size(); ipix++)
    {
        charge(ipix) = charge(ipix) * (*this->cached_correction)[gain_selection[ipix]];
    }
}
