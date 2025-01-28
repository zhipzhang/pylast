/**
 * @file EventMonitor.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Telescope Monitor for each events (Flat-field, pedestal, pixel_status)
 * @version 0.1
 * @date 2025-01-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #pragma once
 #include <unordered_map>
 #include "Eigen/Dense"
 #include <optional>
 #include <TelMonitor.hh>
 #include <BaseTelContainer.hh>
 class EventMonitor: public BaseTelContainer<TelMonitor>
 {
   public:
    EventMonitor() = default;
    virtual ~EventMonitor() = default;
    EventMonitor(EventMonitor&&) = default;
    EventMonitor& operator=(EventMonitor&&) = default;
    void add_telmonitor(int tel_id, int n_channels, int n_pixels, double* pedestal_per_sample, double* dc_to_pe, int max_pixels);
 };

