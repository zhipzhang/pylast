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
 };

