/**
 * @file ArrayEvent.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief core class to describe an array event
 * @version 0.1
 * @date 2024-12-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "SimulatedEvent.hh"
#include <optional>
#include "R0Event.hh"
#include "R1Event.hh"
 class ArrayEvent {
public:
    ArrayEvent() = default;
    ~ArrayEvent() = default;
    ArrayEvent(const ArrayEvent& other) = delete;
    ArrayEvent& operator=(const ArrayEvent& other) = delete;

    ArrayEvent(ArrayEvent&& other) noexcept = default;
    ArrayEvent& operator=(ArrayEvent&& other) noexcept = default;
    std::optional<SimulatedEvent> simulated_event;
    std::optional<R0Event> r0_event;
    //std::optional<R1Event> r1_event;
    inline void add_simulated_camera_image(int tel_id, int n_pixels, int* pe_count, double impact_parameter) 
    {
        if(!simulated_event) {
            throw std::runtime_error("simulated_event must be initialized before adding camera images");
        }
        simulated_event->cameras.emplace(tel_id, SimulatedCamera(n_pixels, pe_count, impact_parameter));
    }
    void add_r0_camera_adc_sample(int tel_id, int n_pixels, int n_samples,  Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> high_gain, Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> low_gain) {
        if(!r0_event) {
            throw std::runtime_error("r0_event must be initialized before adding camera images");
        }
        r0_event->tels[tel_id].set_waveform(n_pixels, n_samples, std::move(high_gain), std::move(low_gain));
    }
    void add_r0_camera_adc_sum(int tel_id, int n_pixels, Eigen::Vector<uint32_t, -1> high_gain_sum, Eigen::Vector<uint32_t, -1> low_gain_sum) {
        if(!r0_event) {
            throw std::runtime_error("r0_event must be initialized before adding camera images");
        }
        r0_event->tels[tel_id].set_waveform_sum(n_pixels, std::move(high_gain_sum), std::move(low_gain_sum));
    }



};