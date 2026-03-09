#include "nlohmann_json/json.hpp"
#include <memory>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <vector>
#include "ArrayEvent.hh"
#include "spdlog/spdlog.h"


struct ModelRange {
    double offset_low;
    double offset_high;
    std::string model_name;  // 如 "cog_estimator_0_1.json"
    
    bool contains(double offset) const {
        return offset >= offset_low && offset < offset_high;
    }
};



template<typename Estimator>
class OffsetEstimator
{
private:
    struct RangeEntry{
        ModelRange range;
        std::unique_ptr<Estimator> estimator;
        std::string full_path;
    };

    std::vector<RangeEntry> ranges_;
    std::string base_directory_;
    nlohmann::json config_;
    RangeEntry* findEntry(double offset) {
        auto it = std::upper_bound(ranges_.begin(), ranges_.end(), offset,
            [](double value, const RangeEntry& entry) {
                return value < entry.range.offset_low;
            });
        
        if (it != ranges_.begin()) {
            --it;
            if (it->range.contains(offset)) {
                return &(*it);
            }
        }
        return nullptr;
    }
public:
    OffsetEstimator(const std::string& config_path)
    {
        std::ifstream file(config_path);
        if(!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + config_path);
        }
        base_directory_ = std::filesystem::path(config_path).parent_path().string();
        config_ = nlohmann::json::parse(file);
        load_config();
    }
    void load_config()
    {
        for(const auto& m: config_["models"])
        {
            RangeEntry entry;
            entry.range.offset_low = m["offset_low"].get<int>();
            entry.range.offset_high = m["offset_high"].get<int>();
            entry.range.model_name = m["model_name"].get<std::string>();
            entry.full_path = base_directory_ + "/" + entry.range.model_name;
            entry.estimator = std::make_unique<Estimator>(entry.full_path);
            ranges_.push_back(std::move(entry));
        }
        std::sort(ranges_.begin(), ranges_.end(), [](const RangeEntry& a, const RangeEntry& b) {
            return a.range.offset_low < b.range.offset_low;
        });
        spdlog::info("Loaded {} models", ranges_.size());
    }
    ~OffsetEstimator() = default;
    template<typename... Args>
    double predict(double offset, Args&&... args)
    {
        auto entry = findEntry(offset);
        if(entry == nullptr)
        {
            entry = &(ranges_.back());
            spdlog::warn("Offset out of range: {}, using last predictor", offset);
        }
        return entry->estimator->predict(std::forward<Args>(args)...);
    }
};