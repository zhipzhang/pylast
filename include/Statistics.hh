/**
 * @file Statistics.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  store the statistics histogram
 * @version 0.1
 * @date 2025-03-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #pragma once

 #include "histogram.hpp"
 #include <unordered_map>
 #include <variant>
 #include <memory>

 using namespace eigen_histogram;
 class Statistics
 {
    public:
        Statistics() = default;
        ~Statistics() = default;

        std::unordered_map<std::string, std::shared_ptr<Histogram<float>>> histograms;
        
        Statistics& operator+=(const Statistics& other) {
            if (histograms.empty())
            {
                histograms = other.histograms;
            }
            else
            {
                for (const auto& [key, value] : other.histograms)
                {
                    if (histograms.find(key) != histograms.end())
                    {
                        *histograms.at(key) =  *histograms.at(key) + *value;
                    }
                }
            }
            return *this;
        }
        template<typename HistType>
        void add_histogram(const std::string& name, HistType histogram) {
            histograms.emplace(name, std::make_shared<HistType>(std::move(histogram)));
        }
        
        // You could also add convenience methods like:
        template<typename T>
        void fill_1d(const std::string& name, T value, float weight = 1.0f) {
            auto hist_ptr = std::dynamic_pointer_cast<Histogram1D<float>>(histograms.at(name));
            if (hist_ptr) {
                hist_ptr->fill(static_cast<float>(value), weight);
            }
        }
        
        template<typename T1, typename T2>
        void fill_2d(const std::string& name, T1 x_value, T2 y_value, float weight = 1.0f) {
            auto hist_ptr = std::dynamic_pointer_cast<Histogram2D<float>>(histograms.at(name));
            if (hist_ptr) {
                hist_ptr->fill(static_cast<float>(x_value), static_cast<float>(y_value), weight);
            }
        }
 };