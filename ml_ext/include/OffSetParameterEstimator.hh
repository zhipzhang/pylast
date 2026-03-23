#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <limits>
#include "spdlog/spdlog.h"
#include <stdexcept>
#include "nlohmann_json/json.hpp"
#include <filesystem>

using json = nlohmann::json;

template<typename Estimator>
class OffsetParameterEstimator
{
public:
    OffsetParameterEstimator() = default;
    OffsetParameterEstimator(const OffsetParameterEstimator&) = delete;
    OffsetParameterEstimator& operator=(const OffsetParameterEstimator&) = delete;

    OffsetParameterEstimator(OffsetParameterEstimator&&) noexcept = default;
    OffsetParameterEstimator& operator=(OffsetParameterEstimator&&) noexcept = default;
    OffsetParameterEstimator(const std::string& config_path)
    {
        if(std::filesystem::exists(config_path))
        {
            std::ifstream file(config_path);
            if(!file.is_open())
            {
                throw std::runtime_error("Failed to open file: " + config_path);
            }
            spdlog::info("Loading offset parameter estimator from file: {}", config_path);
            json config = json::parse(file);
            initialize(config);
        }
        else
        {
            json config = json::parse(config_path);
            initialize(config);
        }
    }

    OffsetParameterEstimator(const json& config)
    {
        initialize(config);
    }

    ~OffsetParameterEstimator() = default;

    // Get the appropriate estimator for a given offset value
    const Estimator& get_estimator(double offset) const
    {
        if(offset < offset_start)
        {
            throw std::runtime_error("Offset out of range: " + std::to_string(offset));
        }
        int bin_index = (offset - offset_start) / offset_bin_width;
        if(bin_index >= num_bins)
        {
            bin_index = num_bins - 1;
        }
        return estimators_[bin_index];
    }
    template<typename... Args>
    double predict(double offset, Args&&... args)
    {
        const auto& estimator = get_estimator(offset);
        return estimator.predict(std::forward<Args>(args)...);
    }

private:
    double offset_start;
    double offset_bin_width;
    int num_bins;
    std::vector<Estimator> estimators_;
    std::string base_directory;
    void initialize(const json& config)
    {
        // Parse create_time
        if(config.contains("create_time"))
        {
            std::string time_str = config.at("create_time").get<std::string>();
            create_time_ = parse_time(time_str);
        }
        if(!config.contains("offset_start"))
        {
            throw std::runtime_error("Config does not contain 'offset_start'");
        }
        else
        {
            offset_start = config.at("offset_start").get<double>();
        }
        if(!config.contains("offset_bin_width"))
        {
            throw std::runtime_error("Config does not contain 'offset_bin_width'");
        }
        else
        {
            offset_bin_width = config.at("offset_bin_width").get<double>();
        }

        if(!config.contains("offset_bins"))
        {
            throw std::runtime_error("Config does not contain 'offset_bins'");
        }
        auto offset_bins_json = config.at("offset_bins");
        if(!offset_bins_json.is_array())
        {
            throw std::runtime_error("'offset_bins' must be an array");
        }
        base_directory = config.at("base_dir").get<std::string>() + "/";
        num_bins = offset_bins_json.size();
        // Process each offset bin
        for(const auto& bin_json : offset_bins_json)
        {
            // Parse model config and create estimator
            if(!bin_json.contains("model"))
            {
                throw std::runtime_error("Offset bin missing 'model' field");
            }

            json model_config = bin_json.at("model");
            estimators_.push_back(std::move(Estimator(base_directory, model_config)));
        }
    }

    time_t parse_time(const std::string& time_str)
    {
        // Parse format: "2025-11-04 19:31:38"
        std::tm tm = {};
        std::istringstream ss(time_str);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if(ss.fail())
        {
            throw std::runtime_error("Failed to parse time: " + time_str);
        }
        return std::mktime(&tm);
    }
    time_t create_time_;
};