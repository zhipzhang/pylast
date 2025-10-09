/**
 * @file ConfigMacros.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Simplified macros for the configuration system
 * @version 0.1
 * @date 2025-09-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include "ConfigSystem.hh"
using nlohmann::json;
// Standard constructors for configurable classes
#define CONFIG_CONSTRUCTORS(ClassType) \
    ClassType() {  initialize(); setUp(); } \
    ClassType(const json& config): config::Configurable(config) { initialize(); setUp(); }  \
    ClassType(const std::string& config_str): config::Configurable(config_str, nullptr) { initialize(); setUp();  } \
    ClassType(const std::string& config_str, std::function<void(const std::string&)> callback): \
        config::Configurable(config_str, callback) {  initialize(); setUp(); }

// Constructors for configurable classes with one additional parameter
#define CONFIG_PARAM_CONSTRUCTORS(ClassType, ParamType, ParamName) \
    ClassType(ParamType ParamName): config::Configurable(), ParamName(ParamName) {initialize(); setUp();  } \
    ClassType(ParamType ParamName, const json& config): \
        config::Configurable(config), ParamName(ParamName) { initialize();setUp();  } \
    ClassType(ParamType ParamName, const std::string& config_str): \
        config::Configurable(config_str, nullptr), ParamName(ParamName) { initialize();setUp();  } \
    ClassType(ParamType ParamName, const std::string& config_str, std::function<void(const std::string&)> callback): \
        config::Configurable(config_str, callback), ParamName(ParamName) { initialize();setUp();  }



// Constructors for configurable classes with two additional parameters
#define CONFIG_DOUBLE_PARAM_CONSTRUCTORS(ClassType, ParamType1, ParamName1, ParamType2, ParamName2) \
    ClassType(ParamType1 ParamName1, ParamType2 ParamName2): \
        config::Configurable(), ParamName1(ParamName1), ParamName2(ParamName2) { initialize();setUp();  } \
    ClassType(ParamType1 ParamName1, ParamType2 ParamName2, const json& config): \
        config::Configurable(config), ParamName1(ParamName1), ParamName2(ParamName2) { initialize();setUp();  } \
    ClassType(ParamType1 ParamName1, ParamType2 ParamName2, const std::string& config_str): \
        config::Configurable(config_str, nullptr), ParamName1(ParamName1), ParamName2(ParamName2) {initialize(); setUp();   } \
    ClassType(ParamType1 ParamName1, ParamType2 ParamName2, const std::string& config_str, \
              std::function<void(const std::string&)> callback): \
        config::Configurable(config_str, callback), ParamName1(ParamName1), ParamName2(ParamName2) { initialize();setUp();  }
        