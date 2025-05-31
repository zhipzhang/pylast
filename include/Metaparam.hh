/**
 * @file Metaparam.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  store the metadata from the simtel file or actual DAQ
 * @version 0.1
 * @date 2024-12-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */

 #pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>

class Metaparam {
public:
    std::unordered_map<std::string, std::string> global_metadata;
    std::unordered_map<int, std::unordered_map<std::string, std::string>> tel_metadata;
    std::vector<std::pair<time_t, std::string>> history;
    std::unordered_map<int, std::vector<std::pair<time_t, std::string>>> tel_history;

    Metaparam() = default;
    void add_global_metadata(const std::string& key, const std::string& value);
    void add_tel_metadata(int tel_id, const std::string& key, const std::string& value);
    void add_history(time_t time, const std::string& history_entry);
    void add_tel_history(int tel_id, time_t time, const std::string& history_entry);
    void print_tel_metadata(int tel_id);
    void print_global_metadata();
    void print_history();
    const std::string print();
};
