#pragma once
#include "DataWriter.hh"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <spdlog/spdlog.h>
class DataWriterFactory {
public:
    using Creator = std::function<std::unique_ptr<FileWriter>(EventSource&, const std::string&)>;
    
    static DataWriterFactory& instance() {
        static DataWriterFactory factory;
        return factory;
    }

    void register_writer(const std::string& type, Creator creator) {
        spdlog::debug("Registering writer type: {}", type);
        creators_[type] = creator;
    }

    std::unique_ptr<FileWriter> create(const std::string& type, EventSource& source, const std::string& filename) {
        auto it = creators_.find(type);
        if (it == creators_.end()) {
            throw std::runtime_error("Unknown writer type: " + type);
        }
        return it->second(source, filename);
    }

private:
    DataWriterFactory() = default;
    std::map<std::string, Creator> creators_;
};

// 方便注册的宏
#define REGISTER_WRITER(type, creator_function) \
    static bool registered_##type = []() { \
        DataWriterFactory::instance().register_writer(#type, \
            creator_function); \
        return true; \
    }(); 
