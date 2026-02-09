#include <fstream>
#include <vector>
#include "ParameterSchema.hh"
using field_registry = std::unordered_map<std::string, FieldEntry>; 
static const field_registry dl1_tel_fields = {
    {"hillas_length", REGISTER_DL1_TEL_FIELD("hillas_length", image_parameters.hillas.length)},
    {"hillas_width", REGISTER_DL1_TEL_FIELD("hillas_width", image_parameters.hillas.width)},
    {"hillas_shape", FieldEntry{"hillas_shape", [](const ArrayEvent& event, int tel_id) -> double {
        return static_cast<double>(event.dl1->tels.at(tel_id)->image_parameters.hillas.width / event.dl1->tels.at(tel_id)->image_parameters.hillas.length);
    }}},
    {"hillas_psi", REGISTER_DL1_TEL_FIELD("hillas_psi", image_parameters.hillas.psi)},
    {"hillas_x", REGISTER_DL1_TEL_FIELD("hillas_x", image_parameters.hillas.x)},
    {"hillas_y", REGISTER_DL1_TEL_FIELD("hillas_y", image_parameters.hillas.y)},
    {"hillas_skewness", REGISTER_DL1_TEL_FIELD("hillas_skewness", image_parameters.hillas.skewness)},
    {"hillas_kurtosis", REGISTER_DL1_TEL_FIELD("hillas_kurtosis", image_parameters.hillas.kurtosis)},
    {"hillas_intensity", REGISTER_DL1_TEL_FIELD("hillas_intensity", image_parameters.hillas.intensity)},
    {"log_intensity", FieldEntry{"log_intensity", [](const ArrayEvent& event, int tel_id) -> double {
        return std::log10(event.dl1->tels.at(tel_id)->image_parameters.hillas.intensity);
    }}},
    {"hillas_r", REGISTER_DL1_TEL_FIELD("hillas_r", image_parameters.hillas.r)},
    {"hillas_phi", REGISTER_DL1_TEL_FIELD("hillas_phi", image_parameters.hillas.phi)},
    {"leakage_pixels_width_1", REGISTER_DL1_TEL_FIELD("leakage_pixels_width_1", image_parameters.leakage.pixels_width_1)},
    {"leakage_pixels_width_2", REGISTER_DL1_TEL_FIELD("leakage_pixels_width_2", image_parameters.leakage.pixels_width_2)},
    {"leakage_intensity_width_1", REGISTER_DL1_TEL_FIELD("leakage_intensity_width_1", image_parameters.leakage.intensity_width_1)},
    {"leakage_intensity_width_2", REGISTER_DL1_TEL_FIELD("leakage_intensity_width_2", image_parameters.leakage.intensity_width_2)},
    {"concentration_cog", REGISTER_DL1_TEL_FIELD("concentration_cog", image_parameters.concentration.concentration_cog)},
    {"concentration_core", REGISTER_DL1_TEL_FIELD("concentration_core", image_parameters.concentration.concentration_core)},
    {"concentration_pixel", REGISTER_DL1_TEL_FIELD("concentration_pixel", image_parameters.concentration.concentration_pixel)},
    {"morphology_n_pixels", REGISTER_DL1_TEL_FIELD("morphology_n_pixels", image_parameters.morphology.n_pixels)},
    {"morphology_n_islands", REGISTER_DL1_TEL_FIELD("morphology_n_islands", image_parameters.morphology.n_islands)},
    {"morphology_n_small_islands", REGISTER_DL1_TEL_FIELD("morphology_n_small_islands", image_parameters.morphology.n_small_islands)},
    {"morphology_n_medium_islands", REGISTER_DL1_TEL_FIELD("morphology_n_medium_islands", image_parameters.morphology.n_medium_islands)},
    {"morphology_n_large_islands", REGISTER_DL1_TEL_FIELD("morphology_n_large_islands", image_parameters.morphology.n_large_islands)},
    {"intensity_max", REGISTER_DL1_TEL_FIELD("intensity_max", image_parameters.intensity.intensity_max)},
    {"intensity_mean", REGISTER_DL1_TEL_FIELD("intensity_mean", image_parameters.intensity.intensity_mean)},
    {"intensity_std", REGISTER_DL1_TEL_FIELD("intensity_std", image_parameters.intensity.intensity_std)},
    {"intensity_skewness", REGISTER_DL1_TEL_FIELD("intensity_skewness", image_parameters.intensity.intensity_skewness)},
    {"intensity_kurtosis", REGISTER_DL1_TEL_FIELD("intensity_kurtosis", image_parameters.intensity.intensity_kurtosis)},
    {"extra_miss", REGISTER_DL1_TEL_FIELD("extra_miss", image_parameters.extra.miss)},
    {"extra_disp", REGISTER_DL1_TEL_FIELD("extra_disp", image_parameters.extra.disp)},
    {"extra_theta", REGISTER_DL1_TEL_FIELD("extra_theta", image_parameters.extra.theta)},
    {"extra_true_psi", REGISTER_DL1_TEL_FIELD("extra_true_psi", image_parameters.extra.true_psi)},
    {"extra_cog_err", REGISTER_DL1_TEL_FIELD("extra_cog_err", image_parameters.extra.cog_err)},
    {"extra_beta_err", REGISTER_DL1_TEL_FIELD("extra_beta_err", image_parameters.extra.beta_err)},
};

static const field_registry dl2_tel_fields = {
    {"rec_impact_parameter", REGISTER_DL2_TEL_FIELD("rec_impact_parameter", impact_parameters.at("HillasReconstructor").distance)},
    {"tel_rec_energy", FieldEntry{"tel_rec_energy", [](const ArrayEvent& event, int tel_id) -> double {
        return pow(10, event.dl2->tels.at(tel_id).estimate_energy);
    }}},
    {"rec_energy", REGISTER_DL2_EVENT_FIELD("rec_energy", energy.at("EnergyRegressor").estimate_energy)},
    {"n_tel", REGISTER_DL2_EVENT_FIELD("n_tel",tels.size())},
    {"rec_energy_std", REGISTER_DL2_EVENT_FIELD("rec_energy_std", energy.at("EnergyRegressor").estimate_energy_std)},
    
};

static const field_registry simulated_fields = {
    {"hillas_length", REGISTER_SIMULATION_TEL_FIELD("hillas_length", image_parameters.hillas.length)},
    {"hillas_width", REGISTER_SIMULATION_TEL_FIELD("hillas_width", image_parameters.hillas.width)},
    {"hillas_shape", FieldEntry{"hillas_shape", [](const ArrayEvent& event, int tel_id) -> double {
        return static_cast<double>(event.simulation->tels.at(tel_id)->image_parameters.hillas.width / event.simulation->tels.at(tel_id)->image_parameters.hillas.length);
    }}},
    {"hillas_psi", REGISTER_SIMULATION_TEL_FIELD("hillas_psi", image_parameters.hillas.psi)},
    {"hillas_x", REGISTER_SIMULATION_TEL_FIELD("hillas_x", image_parameters.hillas.x)},
    {"hillas_y", REGISTER_SIMULATION_TEL_FIELD("hillas_y", image_parameters.hillas.y)},
    {"hillas_skewness", REGISTER_SIMULATION_TEL_FIELD("hillas_skewness", image_parameters.hillas.skewness)},
    {"hillas_kurtosis", REGISTER_SIMULATION_TEL_FIELD("hillas_kurtosis", image_parameters.hillas.kurtosis)},
    {"hillas_intensity", REGISTER_SIMULATION_TEL_FIELD("hillas_intensity", image_parameters.hillas.intensity)},
    {"log_intensity", FieldEntry{"log_intensity", [](const ArrayEvent& event, int tel_id) -> double {
        return std::log10(event.simulation->tels.at(tel_id)->image_parameters.hillas.intensity);
    }}},
    {"hillas_r", REGISTER_SIMULATION_TEL_FIELD("hillas_r", image_parameters.hillas.r)},
    {"hillas_phi", REGISTER_SIMULATION_TEL_FIELD("hillas_phi", image_parameters.hillas.phi)},
    {"leakage_pixels_width_1", REGISTER_SIMULATION_TEL_FIELD("leakage_pixels_width_1", image_parameters.leakage.pixels_width_1)},
    {"leakage_pixels_width_2", REGISTER_SIMULATION_TEL_FIELD("leakage_pixels_width_2", image_parameters.leakage.pixels_width_2)},
    {"leakage_intensity_width_1", REGISTER_SIMULATION_TEL_FIELD("leakage_intensity_width_1", image_parameters.leakage.intensity_width_1)},
    {"leakage_intensity_width_2", REGISTER_SIMULATION_TEL_FIELD("leakage_intensity_width_2", image_parameters.leakage.intensity_width_2)},
    {"concentration_cog", REGISTER_SIMULATION_TEL_FIELD("concentration_cog", image_parameters.concentration.concentration_cog)},
    {"concentration_core", REGISTER_SIMULATION_TEL_FIELD("concentration_core", image_parameters.concentration.concentration_core)},
    {"concentration_pixel", REGISTER_SIMULATION_TEL_FIELD("concentration_pixel", image_parameters.concentration.concentration_pixel)},
    {"morphology_n_pixels", REGISTER_SIMULATION_TEL_FIELD("morphology_n_pixels", image_parameters.morphology.n_pixels)},
    {"morphology_n_islands", REGISTER_SIMULATION_TEL_FIELD("morphology_n_islands", image_parameters.morphology.n_islands)},
    {"morphology_n_small_islands", REGISTER_SIMULATION_TEL_FIELD("morphology_n_small_islands", image_parameters.morphology.n_small_islands)},
    {"morphology_n_medium_islands", REGISTER_SIMULATION_TEL_FIELD("morphology_n_medium_islands", image_parameters.morphology.n_medium_islands)},
    {"morphology_n_large_islands", REGISTER_SIMULATION_TEL_FIELD("morphology_n_large_islands", image_parameters.morphology.n_large_islands)},
    {"intensity_max", REGISTER_SIMULATION_TEL_FIELD("intensity_max", image_parameters.intensity.intensity_max)},
    {"intensity_mean", REGISTER_SIMULATION_TEL_FIELD("intensity_mean", image_parameters.intensity.intensity_mean)},
    {"intensity_std", REGISTER_SIMULATION_TEL_FIELD("intensity_std", image_parameters.intensity.intensity_std)},
    {"intensity_skewness", REGISTER_SIMULATION_TEL_FIELD("intensity_skewness", image_parameters.intensity.intensity_skewness)},
    {"intensity_kurtosis", REGISTER_SIMULATION_TEL_FIELD("intensity_kurtosis", image_parameters.intensity.intensity_kurtosis)},
    {"extra_miss", REGISTER_SIMULATION_TEL_FIELD("extra_miss", image_parameters.extra.miss)},
    {"extra_disp", REGISTER_SIMULATION_TEL_FIELD("extra_disp", image_parameters.extra.disp)},
    {"extra_theta", REGISTER_SIMULATION_TEL_FIELD("extra_theta", image_parameters.extra.theta)},
    {"extra_true_psi", REGISTER_SIMULATION_TEL_FIELD("extra_true_psi", image_parameters.extra.true_psi)},
    {"extra_cog_err", REGISTER_SIMULATION_TEL_FIELD("extra_cog_err", image_parameters.extra.cog_err)},
    {"extra_beta_err", REGISTER_SIMULATION_TEL_FIELD("extra_beta_err", image_parameters.extra.beta_err)},
};



FeatureSchema::FeatureSchema(const json& config) {
    for(const auto& spec : config) {
        FeatureSpec s;
        s.name = spec.at("name").get<std::string>();
        s.level = ParseDataLevel(spec.at("level").get<std::string>());
        s.description = spec.at("description").get<std::string>();
        specs.push_back(s);
    }
}
std::vector<double> TelFeatureExtractor::extract_tel_features(const ArrayEvent& event, int tel_id) {
    std::vector<double> features;
    for(const auto& field : schema.specs) {
        if(field.level == DataLevel::DL1) {
            if(!dl1_tel_fields.contains(field.name)) {
                throw std::runtime_error("Field " + field.name + " not found in DL1 tel fields");
            }
            features.push_back(dl1_tel_fields.at(field.name).get(event, tel_id));
        }
        else if(field.level == DataLevel::DL2) {
            if(!dl2_tel_fields.contains(field.name)) {
                throw std::runtime_error("Field " + field.name + " not found in DL2 tel fields");
            }
            features.push_back(dl2_tel_fields.at(field.name).get(event, tel_id));
        }
        else if(field.level == DataLevel::Simulation) {
            if(!simulated_fields.contains(field.name)) {
                throw std::runtime_error("Field " + field.name + " not found in simulated fields");
            }
            features.push_back(simulated_fields.at(field.name).get(event, tel_id));
        }
        else {
            throw std::runtime_error("Invalid data level: " + std::to_string(static_cast<int>(field.level)));
        }
    }
    return features;
}