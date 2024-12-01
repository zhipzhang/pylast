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
#include "nanobind/nanobind.h"
#include "nanobind/eigen/dense.h"
#include <spdlog/fmt/fmt.h>

namespace nb = nanobind;

class TableAtmosphereModel   
{
public:
    TableAtmosphereModel() = default;
    ~TableAtmosphereModel() = default;
    TableAtmosphereModel(const std::string& filename):input_filename(filename){}
    TableAtmosphereModel(int n_alt, double* alt_km, double* rho, double* thick, double* refidx_m1);
    TableAtmosphereModel& operator=(TableAtmosphereModel&& other) noexcept;
  
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

    static void bind(nb::module_& m)
    {
        nb::class_<TableAtmosphereModel>(m, "TableAtmosphereModel")
            .def(nb::init<const std::string&>())
            .def(nb::init<int, double*, double*, double*, double*>())
            //.def("get_density", &TableAtmosphereModel::get_density)
            //.def("get_thickness", &TableAtmosphereModel::get_thickness)
            //.def("get_refidx_m1", &TableAtmosphereModel::get_refidx_m1)
            .def_ro("input_filename", &TableAtmosphereModel::input_filename)
            .def_ro("n_alt", &TableAtmosphereModel::n_alt)
            .def_ro("alt_km", &TableAtmosphereModel::alt_km)
            .def_ro("rho", &TableAtmosphereModel::rho)
            .def_ro("thick", &TableAtmosphereModel::thick)
            .def_ro("refidx_m1", &TableAtmosphereModel::refidx_m1)
            .def("__repr__", &TableAtmosphereModel::print);
    }
private:
    int n_alt;
    /** @brief Altitude above sea level in kilometers */
    Eigen::VectorXd alt_km;
    /** @brief Density in g/cm^3 at each altitude level */
    Eigen::VectorXd rho;
    /** @brief Vertical column density from space to given level in g/cm^2 */
    Eigen::VectorXd thick;
    /** @brief Index of refraction minus one (n-1) at each altitude level */
    Eigen::VectorXd refidx_m1;
    std::string input_filename;
};