/**
 * @file BaseTelContainer.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Basic container for telescope data
 * @version 0.1
 * @date 2025-01-16
 * 
 * @copyright Copyright (c) 2025
 * 
 */
 #pragma once
 #include <unordered_map>
 #include <memory>


template<typename TelData>
class BaseTelContainer{
    public:
        BaseTelContainer() = default;
        ~BaseTelContainer() = default;
        BaseTelContainer(const BaseTelContainer& other) = delete;
        BaseTelContainer& operator=(const BaseTelContainer& other) = delete;
        BaseTelContainer(BaseTelContainer&& other) noexcept = default;
        BaseTelContainer& operator=(BaseTelContainer&& other) noexcept = default;
        std::unordered_map<int, std::unique_ptr<TelData>> tels;
        template<typename... Args>
        TelData* add_tel(int tel_id, Args&&... args) {
            if (tels.find(tel_id) != tels.end()) {
                return nullptr;
            }
            auto [it, success] = tels.emplace(
                tel_id, 
                std::make_unique<TelData>(std::forward<Args>(args)...)
            );
            return success ? it->second.get() : nullptr;
        }
        TelData* get_tel(int tel_id) {
            auto it = tels.find(tel_id);
            return it != tels.end() ? it->second.get() : nullptr;
        }
        const TelData* get_tel(int tel_id) const {
            auto it = tels.find(tel_id);
            return it != tels.end() ? it->second.get() : nullptr;
        }
        std::unordered_map<int, TelData*> get_tels() const {
            std::unordered_map<int, TelData*> rels;
            for(auto& pair : tels){
                rels[pair.first] = pair.second.get();
            }
            return rels;
        }
};