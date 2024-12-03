#include "Metaparam.hh"
#include <iostream>
void Metaparam::add_global_metadata(const std::string& key, const std::string& value) {
    global_metadata[key] = value;
}

void Metaparam::add_tel_metadata(int tel_id, const std::string& key, const std::string& value) {
    tel_metadata[tel_id][key] = value;
}
void Metaparam::add_history(time_t time, const std::string& history_entry) {
    history.push_back(std::make_pair(time, history_entry));
}
void Metaparam::print_tel_metadata(int tel_id) {
    for(const auto& [key, value] : tel_metadata[tel_id]) {
        std::cout << key << " : " << value << std::endl;
    }
}

void Metaparam::print_global_metadata() {
    for(const auto& [key, value] : global_metadata) {
        std::cout << key << " : " << value << std::endl;
    }
}

void Metaparam::print_history() {
    for(const auto& [time, entry] : history) {
        std::cout << time << " : " << entry << std::endl;
    }
}

void Metaparam::add_tel_history(int tel_id, time_t time, const std::string& history_entry) {
    tel_history[tel_id].push_back(std::make_pair(time, history_entry));
}