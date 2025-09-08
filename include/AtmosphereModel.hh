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
#include "rpolator.h"


class TableAtmosphereModel   
{
public:
    TableAtmosphereModel() = default;
    ~TableAtmosphereModel() {
        if(cs_thick) {
            free(cs_thick);
            cs_thick = nullptr;
        }
    }
    TableAtmosphereModel(const std::string& filename);
    TableAtmosphereModel(int n_alt, double* alt_km, double* rho, double* thick, double* refidx_m1);
    TableAtmosphereModel(const TableAtmosphereModel& other)
    {
        n_alt = other.n_alt;
        alt_km = other.alt_km;
        rho = other.rho;
        thick = other.thick;
        refidx_m1 = other.refidx_m1;
        input_filename = other.input_filename;
        cs_thick = set_1d_cubic_params(alt_km.data(), thick.data(), n_alt, 0);
    }
    TableAtmosphereModel& operator=(const TableAtmosphereModel& other)
    {
        if(this != &other)
        {
            n_alt = other.n_alt;
            alt_km = other.alt_km;
            rho = other.rho;
            thick = other.thick;
            refidx_m1 = other.refidx_m1;
            input_filename = other.input_filename;
            cs_thick = set_1d_cubic_params(alt_km.data(), thick.data(), n_alt, 0);
        }
        return *this;
    }
    static TableAtmosphereModel& global_instance() {
        static TableAtmosphereModel instance;
        return instance;
    }
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
    Eigen::VectorXd convert_hmax_to_xmax(const Eigen::VectorXd& hmax);
    double convert_hmax_to_xmax(double hmax);
private:
    CsplinePar* cs_thick = nullptr;  /**< Cubic spline parameters for thickness vs. altitude */
};