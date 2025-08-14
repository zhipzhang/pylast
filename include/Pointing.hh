/**
 * @file Pointing.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief store the pointing information for each event/each telescope
 * @version 0.1
 * @date 2025-02-25
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #pragma once
 #include "BaseTelContainer.hh"
 
 class PointingTelescope
 {
    public:
        double azimuth;
        double altitude;
 };
 class Pointing: public BaseTelContainer<PointingTelescope>
 {
    public:
        Pointing() = default;
        double array_azimuth;
        double array_altitude;
        void set_array_pointing(double azimuth, double altitude)
        {
            array_azimuth = azimuth;
            array_altitude = altitude;
        }
 };