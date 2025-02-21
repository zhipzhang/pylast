/**
 * @file Configurable.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2025-02-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include "nlohmann_json/json.hpp"
#include <fstream>

using json = nlohmann::json;
class Configurable
{
public:
    Configurable() = default;
    Configurable(const json& config): config(config) {}
    Configurable(const std::string& config_str): config(from_string(config_str)) {}

    void initialize()
    {
        final_config = default_config();
        if(!config.empty())
        {
            final_config.merge_patch(config);
        }
        configure(final_config);
    }

    virtual void configure(const json& config) = 0;
    
    virtual json default_config() const = 0;

    static json from_file(const std::string& filename)
    {
        std::ifstream file(filename);
        if(!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        return json::parse(file);
    }

    static json from_string(const std::string& str)
    {
        return json::parse(str);
    }

    virtual ~Configurable() = default;

protected:
    json final_config;
    json config;
};

// ... existing code ...

// Add this macro definition before the end of the file

#define DECLARE_CONFIGURABLE_CONSTRUCTORS(ClassType) \
    ClassType() { initialize(); } \
    ClassType(const json& config): Configurable(config) { initialize(); } \
    ClassType(const std::string& config_str): Configurable(config_str) { initialize(); }

// 修改宏定义以支持不同的参数类型和名称
#define DECLARE_CONFIGURABLE_DEFINITIONS(ParamType, ParamName, ClassType) \
    ClassType(ParamType ParamName): Configurable(), ParamName(ParamName) { initialize(); } \
    ClassType(ParamType ParamName, const json& config): Configurable(config), ParamName(ParamName) { initialize(); } \
    ClassType(ParamType ParamName, const std::string& config_str): Configurable(config_str), ParamName(ParamName) { initialize(); }

#define DECLARE_CONFIGURABLE_DOUBLE_DEFINITIONS(ParamType1, ParamName1, ParamType2, ParamName2, ClassType) \
    ClassType(ParamType1 ParamName1, ParamType2 ParamName2): Configurable(), ParamName1(ParamName1), ParamName2(ParamName2) { initialize(); } \
    ClassType(ParamType1 ParamName1, ParamType2 ParamName2, const json& config): Configurable(config), ParamName1(ParamName1), ParamName2(ParamName2) { initialize(); } \
    ClassType(ParamType1 ParamName1, ParamType2 ParamName2, const std::string& config_str): Configurable(config_str), ParamName1(ParamName1), ParamName2(ParamName2) { initialize(); }

