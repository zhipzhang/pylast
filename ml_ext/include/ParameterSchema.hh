#pragma once
#include "ArrayEvent.hh"
#include <string>
#include <stdexcept>
#include "nlohmann_json/json.hpp"


using json = nlohmann::json;
enum class DataLevel{ Simulation, DL1, DL2};
struct FeatureSpec{
    std::string name;
    DataLevel level;
    std::string description;
};

static DataLevel ParseDataLevel(const std::string& level){
    if(level == "Simulation" || level == "simulation"){
        return DataLevel::Simulation;
    }
    else if(level == "DL1" || level == "dl1"){
        return DataLevel::DL1;
    }
    else if(level == "DL2" || level == "dl2"){
        return DataLevel::DL2;
    }
    else{
        throw std::runtime_error("Invalid data level: " + level);
    }
}

struct FeatureSchema{
    FeatureSchema(const json& config);
    std::vector<FeatureSpec> specs;
    int NumFeatures() const {
        return specs.size();
    }
};

struct TelFeatureExtractor{
    TelFeatureExtractor(const json& config):schema(config){}
    int GetFeatureNumber() const {
        return schema.NumFeatures();
    }
    FeatureSchema schema;
    std::vector<double> extract_tel_features(const ArrayEvent& event, int tel_id);
};


struct FieldEntry
{
    std::string name;
    std::function<double(const ArrayEvent& , int tel_id)> get;
};

#define REGISTER_DL1_TEL_FIELD(name, path) \
    FieldEntry{name, [](const ArrayEvent& event, int tel_id) -> double { \
        return static_cast<double>(event.dl1->tels.at(tel_id)->path); \
    }}
#define REGISTER_DL2_TEL_FIELD(name, path) \
    FieldEntry{name, [](const ArrayEvent& event, int tel_id) -> double { \
        return static_cast<double>(event.dl2->tels.at(tel_id).path); \
    }}
#define REGISTER_DL2_EVENT_FIELD(name, path) \
    FieldEntry{name, [](const ArrayEvent& event, int tel_id) -> double { \
        return static_cast<double>(event.dl2->path); \
    }}
#define REGISTER_SIMULATION_TEL_FIELD(name, path) \
    FieldEntry{name, [](const ArrayEvent& event, int tel_id) -> double { \
        return static_cast<double>(event.simulation->tels.at(tel_id)->path); \
    }}
