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
 class ArrayEvent {
public:
    ArrayEvent() = default;
    ~ArrayEvent() = default;
    ArrayEvent(const ArrayEvent& other) = delete;
    ArrayEvent& operator=(const ArrayEvent& other) = delete;

    ArrayEvent(ArrayEvent&& other) noexcept = default;
    ArrayEvent& operator=(ArrayEvent&& other) noexcept = default;
    SimulatedEvent simulated_event;

    inline void add_simulated_camera_image(int tel_id, int n_pixels, int* pe_count, double impact_parameter) 
    {
        simulated_event.cameras.emplace(tel_id, SimulatedCamera(n_pixels, pe_count, impact_parameter));
    }


};