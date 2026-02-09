#include "LookupTableParticleClassifier.hh"
#include "histogram_helper.hh"
#include "ReconstructorFactory.hh"

static bool registered_lookup_table_particle_classifier = []() {
    ReconstructorFactory::instance().register_reconstructor(
        "LookupTableParticleClassifier",
        [](const SubarrayDescription &subarray,
           const json &config) -> std::unique_ptr<Reconstructor> {
            return std::make_unique<LookupTableParticleClassifier>(config);
        });
    return true;
}();
void LookupTableParticleClassifier::load_lookup_table(const std::string &lookup_table_path) {
    lookup_table_file = std::make_unique<TFile>(lookup_table_path.c_str(), "READ");
    if (!lookup_table_file->IsOpen()) {
        throw std::runtime_error("Failed to open lookup table file: " + lookup_table_path);
    }
    mean_length_lookup_table = dynamic_cast<TH2D *>(lookup_table_file->Get("mean_length"));
    if (!mean_length_lookup_table) {
        throw std::runtime_error("Failed to get mean length lookup table from file: " + lookup_table_path);
    }
    sigma_length_lookup_table = dynamic_cast<TH2D *>(lookup_table_file->Get("sigma_length"));
    if (!sigma_length_lookup_table) {
        throw std::runtime_error("Failed to get sigma length lookup table from file: " + lookup_table_path);
    }
    mean_width_lookup_table = dynamic_cast<TH2D *>(lookup_table_file->Get("mean_width"));
    if (!mean_width_lookup_table) {
        throw std::runtime_error("Failed to get mean width lookup table from file: " + lookup_table_path);
    }
    sigma_width_lookup_table = dynamic_cast<TH2D *>(lookup_table_file->Get("sigma_width"));
    if (!sigma_width_lookup_table) {
        throw std::runtime_error("Failed to get sigma width lookup table from file: " + lookup_table_path);
    }
}

void LookupTableParticleClassifier::operator()(ArrayEvent &event) {
    Reconstructor::operator()(event);
    if (telescopes.size() < 2) {
        particle_reco.is_valid = false;
        event.dl2->particle[this->name()] = particle_reco;
        return;
    }
    if (!event.dl2->geometry.contains("HillasReconstructor")) {
        throw std::runtime_error("HillasReconsturctor is not avaliable, it's better to have it as initial value for LookupTableParticleClassifier");
    }
    Eigen::VectorXd intensity = Eigen::VectorXd::Zero(telescopes.size());
    Eigen::VectorXd impact_parameters = Eigen::VectorXd::Zero(telescopes.size());
    Eigen::VectorXd lengths = Eigen::VectorXd::Zero(telescopes.size());
    Eigen::VectorXd widths = Eigen::VectorXd::Zero(telescopes.size());
    for(int i = 0; i < telescopes.size(); i++) {
        if (use_fake_hillas) {
            intensity[i] = event.simulation->tels[telescopes[i]]
                         ->image_parameters.hillas.intensity;
            lengths[i] = event.simulation->tels[telescopes[i]]->image_parameters.hillas.length;
            widths[i] = event.simulation->tels[telescopes[i]]->image_parameters.hillas.width;
        } else {
            intensity[i] = event.dl1->tels[telescopes[i]]->image_parameters.hillas.intensity;
            lengths[i] = event.dl1->tels[telescopes[i]]->image_parameters.hillas.length;
            widths[i] = event.dl1->tels[telescopes[i]]->image_parameters.hillas.width;
        }
        impact_parameters[i] = event.dl2->tels[telescopes[i]].impact("HillasReconstructor").distance;
    }
    std::vector<int> valid_telescopes;
    std::vector<double> mrsw;
    std::vector<double> mrsl;
    for(int i = 0; i < telescopes.size(); i++) {
        int tel_id = telescopes[i];
        InterpResult mean_length_result = interpolate_histogram(mean_length_lookup_table, impact_parameters[i], log10(intensity[i]), -999);
        InterpResult sigma_length_result = interpolate_histogram(sigma_length_lookup_table, impact_parameters[i], log10(intensity[i]), -999);
        InterpResult mean_width_result = interpolate_histogram(mean_width_lookup_table, impact_parameters[i], log10(intensity[i]), -999);
        InterpResult sigma_width_result = interpolate_histogram(sigma_width_lookup_table, impact_parameters[i], log10(intensity[i]), -999);
        if (!mean_length_result.inside || !sigma_length_result.inside || !mean_width_result.inside || !sigma_width_result.inside) {
            continue;
        }
        if(sigma_width_result.value == 0 || sigma_length_result.value == 0) {
            spdlog::warn("sigma_width_result.value or sigma_length_result.value is 0, tel_id: {}, impact_parameter: {}, intensity: {}", tel_id, impact_parameters[i], intensity[i]);
            continue;
        }
        double mrsw_value = (widths[i] - mean_width_result.value) / sigma_width_result.value;
        double mrsl_value = (lengths[i] - mean_length_result.value) / sigma_length_result.value;
        mrsw.push_back(mrsw_value);
        mrsl.push_back(mrsl_value);
        valid_telescopes.push_back(tel_id);
    }
    if(mrsw.size() == 0 || mrsl.size() == 0) {
        particle_reco.is_valid = false;
        event.dl2->particle[this->name()] = particle_reco;
        return;
    }
    particle_reco.mrsw = std::accumulate(mrsw.begin(), mrsw.end(), 0.0) / mrsw.size();
    particle_reco.mrsl = std::accumulate(mrsl.begin(), mrsl.end(), 0.0) / mrsl.size();
    particle_reco.is_valid = true;
    particle_reco.telescopes = valid_telescopes;
    event.dl2->particle[this->name()] = particle_reco;
}