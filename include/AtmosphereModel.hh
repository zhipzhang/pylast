/**
 * @file AtmosphereModel.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief class for atmosphere model of corsika input
 * @version 0.1
 * @date 2024-12-01
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include "Eigen/Dense"
#include <spdlog/fmt/fmt.h>


class TableAtmosphereModel   
{
public:
    TableAtmosphereModel() = default;
    TableAtmosphereModel(const std::string& filename);
    TableAtmosphereModel(int n_alt, double* alt_km, double* rho, double* thick, double* refidx_m1);
  
   // double get_density(double altitude) const;
   // double get_thickness(double altitude) const;
   // double get_refidx_m1(double altitude) const;
    
    std::string print() const {
        std::string table = fmt::format("TableAtmosphereModel({})\n", input_filename);
        table += fmt::format("{:>15} {:>15} {:>15} {:>15}\n", 
                             "Altitude (km)", "Density (g/cm³)", "Thickness (g/cm²)", "Refidx-1");
        
        for (int i = 0; i < n_alt; ++i) {
            table += fmt::format("{:15.3f} {:15.6e} {:15.3f} {:15.6e}\n", 
                                 alt_km[i], rho[i], thick[i], refidx_m1[i]);
        }
        return table;
    }

    int n_alt;
    /** @brief Altitude above sea level in kilometers */
    Eigen::VectorXd alt_km;
    /** @brief Density in g/cm^3 at each altitude level */
    Eigen::VectorXd rho;
    /** @brief Vertical column density from space to given level in g/cm^2 */
    Eigen::VectorXd thick;
    /** @brief Index of refraction minus one (n-1) at each altitude level */
    Eigen::VectorXd refidx_m1;
    std::string input_filename = "none";
};