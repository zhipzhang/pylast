/**
 * @file SimulatedShowerArray.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  To read all shower events from file
 * @version 0.1
 * @date 2024-12-12
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include "SimulatedShower.hh"
#include <vector>
#include <Eigen/Dense>

class SimulatedShowerArray {
public:
    SimulatedShowerArray(size_t initial_size = 0) {
        resize(initial_size);
    }
    SimulatedShowerArray(const SimulatedShowerArray& other) = delete;
    SimulatedShowerArray& operator=(const SimulatedShowerArray& other) = delete;
    SimulatedShowerArray(SimulatedShowerArray&& other) noexcept = default;
    SimulatedShowerArray& operator=(SimulatedShowerArray&& other) noexcept = default;
    ~SimulatedShowerArray() = default;
    void resize(size_t new_size) {
        energies.reserve(new_size);
        alts.reserve(new_size);
        azs.reserve(new_size);
        core_xs.reserve(new_size);
        core_ys.reserve(new_size);
        h_first_ints.reserve(new_size);
        x_maxs.reserve(new_size);
        starting_grammages.reserve(new_size);
        shower_primary_ids.reserve(new_size);
    }

    void push_back(const SimulatedShower& shower) {
        energies.push_back(shower.energy);
        alts.push_back(shower.alt);
        azs.push_back(shower.az);
        core_xs.push_back(shower.core_x);
        core_ys.push_back(shower.core_y);
        h_first_ints.push_back(shower.h_first_int);
        x_maxs.push_back(shower.x_max);
        starting_grammages.push_back(shower.starting_grammage);
        shower_primary_ids.push_back(shower.shower_primary_id);
    }

    size_t size() const { return energies.size(); }

    // 属性访问方法，返回 Eigen::Map 避免复制
    Eigen::Map<const Eigen::VectorXd> energy() const { 
        return {energies.data(), static_cast<Eigen::Index>(energies.size())}; 
    }
    Eigen::Map<const Eigen::VectorXd> alt() const { 
        return {alts.data(), static_cast<Eigen::Index>(alts.size())}; 
    }
    Eigen::Map<const Eigen::VectorXd> az() const { 
        return {azs.data(), static_cast<Eigen::Index>(azs.size())}; 
    }
    Eigen::Map<const Eigen::VectorXd> core_x() const { 
        return {core_xs.data(), static_cast<Eigen::Index>(core_xs.size())}; 
    }
    Eigen::Map<const Eigen::VectorXd> core_y() const { 
        return {core_ys.data(), static_cast<Eigen::Index>(core_ys.size())}; 
    }
    Eigen::Map<const Eigen::VectorXd> h_first_int() const { 
        return {h_first_ints.data(), static_cast<Eigen::Index>(h_first_ints.size())}; 
    }
    Eigen::Map<const Eigen::VectorXd> x_max() const { 
        return {x_maxs.data(), static_cast<Eigen::Index>(x_maxs.size())}; 
    }
    Eigen::Map<const Eigen::VectorXd> starting_grammage() const { 
        return {starting_grammages.data(), static_cast<Eigen::Index>(starting_grammages.size())}; 
    }
    Eigen::Map<const Eigen::VectorXi> shower_primary_id() const { 
        return {shower_primary_ids.data(), static_cast<Eigen::Index>(shower_primary_ids.size())}; 
    }

    // 获取单个shower的所有属性
    SimulatedShower at(size_t idx) const {
        if(idx >= size()) {
            throw std::out_of_range("Index out of range");
        }
        SimulatedShower shower;
        shower.energy = energies[idx];
        shower.alt = alts[idx];
        shower.az = azs[idx];
        shower.core_x = core_xs[idx];
        shower.core_y = core_ys[idx];
        shower.h_first_int = h_first_ints[idx];
        shower.x_max = x_maxs[idx];
        shower.starting_grammage = starting_grammages[idx];
        shower.shower_primary_id = shower_primary_ids[idx];
        return shower;
    }
    SimulatedShower operator[](size_t idx) const {
        return at(idx);
    }
    const std::string print() const {
        return fmt::format("SimulatedShowerArray(\n"
                         "    energy: array of {} showers\n"
                         "    alt: array of {} showers\n"
                         "    az: array of {} showers\n"
                         "    core_x: array of {} showers\n"
                         "    core_y: array of {} showers\n"
                         "    h_first_int: array of {} showers\n"
                         "    x_max: array of {} showers\n"
                         "    starting_grammage: array of {} showers\n"
                         "    shower_primary_id: array of {} showers\n"
                         ")",
                         energies.size(), alts.size(), azs.size(),
                         core_xs.size(), core_ys.size(), h_first_ints.size(),
                         x_maxs.size(), starting_grammages.size(), shower_primary_ids.size());
    }

private:
    std::vector<double> energies;
    std::vector<double> alts;
    std::vector<double> azs;
    std::vector<double> core_xs;
    std::vector<double> core_ys;
    std::vector<double> h_first_ints;
    std::vector<double> x_maxs;
    std::vector<double> starting_grammages;
    std::vector<int> shower_primary_ids;
};