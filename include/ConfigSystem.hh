// ConfigSystem.hh
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <unordered_map>
#include <any>
#include <spdlog/spdlog.h>
#include <iostream>

namespace config {

using json = nlohmann::json;



// Configuration parameter definition
struct ConfigParam {
    std::string path;       // Path in JSON (e.g., "TailcutsCleaner.picture_thresh")
    std::any defaultValue;  // Default value
    std::function<void(const std::any&)> setter; // Function to set the value
};

// Base configurable interface
class Configurable {
public:
    Configurable() = default;
    Configurable(const json& config) : userConfig(config) {
    }
    Configurable(const std::string& config_str, std::function<void(const std::string&)> call_back) {
            from_string(config_str, call_back);
    }
    virtual ~Configurable() = default;
    
    // Initialize with configuration
    void initialize() {
        processUserConfig();
        json finalConfig = buildDefaultConfig();
        if (!userConfig.is_null()) {
            finalConfig = mergeConfig(finalConfig, userConfig);
        }
        applyConfig(finalConfig);
        currentConfig = finalConfig;
    }
    
    // Get current configuration
    const json& getConfig() const { return currentConfig; }
    std::string getConfigString(int indent = 2) const { return currentConfig.dump(indent); }
    
protected:
    // Register configuration parameters
    virtual void registerParams() = 0;
    virtual void setUp() = 0;
    
    template<typename T>
    void registerParam(const std::string& path, T defaultValue) {
        params.push_back({
            path,
            std::any(defaultValue),
            [](const std::any& value) {
                // Do nothing
            }
        });
    }
    // Register a single parameter
    template<typename T>
    void registerParam(const std::string& path, T defaultValue, T& variable) {
        params.push_back({
            path,
            std::any(defaultValue),
            [&variable](const std::any& value) {
                variable = std::any_cast<T>(value);
            }
        });
    }
    
    // Build default configuration from registered parameters
    json buildDefaultConfig() {
        params.clear();
        registerParams();
        
        json config;
        for (const auto& param : params) {
            setJsonValue(config, param.path, param.defaultValue);
        }
        return config;
    }
    
    // Apply configuration to registered parameters
    void applyConfig(const json& config) {
        for (const auto& param : params) {
            std::any value;
            if (getJsonValue(config, param.path, param.defaultValue, value)) {
                param.setter(value);
            } else {
                param.setter(param.defaultValue);
            }
        }
    }
    
public:
    void processUserConfig()
    {
        if(userConfig.is_null() || !userConfig.is_object())
        {
            return;
        }
        json processedConfig;
        for( auto[key, value]: userConfig.items()) {
            setJsonValue(processedConfig, key, json_to_any(value));
        }
        userConfig = processedConfig;
    }
    // Helper to set a value in JSON using path notation
    void setJsonValue(json& config, const std::string& path, const std::any& value) {
        auto parts = splitPath(path);
        json* current = &config;
        
        // Navigate through the JSON structure
        for (size_t i = 0; i < parts.size() - 1; ++i) {
            if (!current->contains(parts[i])) {
                (*current)[parts[i]] = json::object();
            }
            current = &(*current)[parts[i]];
        }
        
        // Set the final value based on type
        if (value.type() == typeid(int)) {
            (*current)[parts.back()] = std::any_cast<int>(value);
        } else if (value.type() == typeid(double)) {
            (*current)[parts.back()] = std::any_cast<double>(value);
        } else if (value.type() == typeid(bool)) {
            (*current)[parts.back()] = std::any_cast<bool>(value);
        } else if (value.type() == typeid(std::string)) {
            (*current)[parts.back()] = std::any_cast<std::string>(value);
        } else if (value.type() == typeid(json))
        {
            (*current)[parts.back()] = std::any_cast<json>(value);
        }
    }
    
    // Helper to get a value from JSON using path notation
    bool getJsonValue(const json& config, const std::string& path, 
                      const std::any& defaultValue, std::any& result) {
        auto parts = splitPath(path);
        const json* current = &config;
        
        // Navigate through the JSON structure
        for (size_t i = 0; i < parts.size() - 1; ++i) {
            if (!current->contains(parts[i])) {
                result = defaultValue;
                return false;
            }
            current = &(*current)[parts[i]];
        }
        
        // Get the final value
        if (!current->contains(parts.back())) {
            result = defaultValue;
            return false;
        }
        
        // Convert to the appropriate type
        if (defaultValue.type() == typeid(int)) {
            result = (*current)[parts.back()].get<int>();
        } else if (defaultValue.type() == typeid(double)) {
            result = (*current)[parts.back()].get<double>();
        } else if (defaultValue.type() == typeid(bool)) {
            result = (*current)[parts.back()].get<bool>();
        } else if (defaultValue.type() == typeid(std::string)) {
            result = (*current)[parts.back()].get<std::string>();
        }else if(defaultValue.type() == typeid(json))
        {
            result = (*current)[parts.back()].get<json>();
        } else {
            result = defaultValue;
            return false;
        }
        
        return true;
    }
    
    // Split path string into components
    static std::vector<std::string> splitPath(const std::string& path) {
        std::vector<std::string> result;
        std::string current;
        
        for (char c : path) {
            if (c == '.') {
                if (!current.empty()) {
                    result.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        
        if (!current.empty()) {
            result.push_back(current);
        }
        
        return result;
    }
    static std::any json_to_any(const json& j)
    {
        if (j.is_null())      return std::any(nullptr);
        if (j.is_boolean())   return std::any(j.get<bool>());
        if (j.is_number_integer())   return std::any(j.get<int>());
        if (j.is_number_float())     return std::any(j.get<double>());
        if (j.is_string())    return std::any(j.get<std::string>());
        return std::any(j);   
    }
    
    // Merge configurations
    json mergeConfig(const json& base, const json& override) {
        json result = base;
        result.merge_patch(override);
        return result;
    }
    
    // Parse JSON string and apply configuration
    void from_string(const std::string& config_str, std::function<void(const std::string&)> call_back) {
        try {
                userConfig =  json::parse(config_str);
        } catch (const json::parse_error& e) {
            if(call_back) {
                call_back(config_str);
            }
            else
            {
                throw std::runtime_error("JSON parse error: " + std::string(e.what()));
            }
        }
    }
    
private:
    json userConfig;     // User-provided configuration
    json currentConfig;  // Current active configuration
    std::vector<ConfigParam> params; // Registered parameters
};

} // namespace config