#include "LookupTableEnergyRegressor.hh"
#include "ReconstructorFactory.hh"
#include "TFile.h"
#include "TH2D.h"
#include "histogram_helper.hh"
static bool registered_lookup_table_energy_regressor = []() {
  ReconstructorFactory::instance().register_reconstructor(
      "LookupTableEnergyRegressor",
      [](const SubarrayDescription &subarray,
         const json &config) -> std::unique_ptr<Reconstructor> {
        return std::make_unique<LookupTableEnergyRegressor>(config);
      });
  return true;
}();

void LookupTableEnergyRegressor::load_lookup_table(
    const std::string &lookup_table_path) {
  lookup_table_file =
      std::make_unique<TFile>(lookup_table_path.c_str(), "READ");
  if (!lookup_table_file->IsOpen()) {
    throw std::runtime_error("Failed to open lookup table file: " +
                             lookup_table_path);
  }
  mean_lookup_table =
      dynamic_cast<TH2D *>(lookup_table_file->Get("mean_energy_regression"));
  if (!mean_lookup_table) {
    throw std::runtime_error("Failed to get lookup table from file: " +
                             lookup_table_path);
  }
  sigma_lookup_table =
      dynamic_cast<TH2D *>(lookup_table_file->Get("sigma_energy_regression"));
  if (!sigma_lookup_table) {
    spdlog::warn("Failed to get sigma lookup table from file: " +
                 lookup_table_path);
    spdlog::warn("Using Intensity as weight instead");
  } else {
    use_sigma_as_weight = true;
  }
}
void LookupTableEnergyRegressor::operator()(ArrayEvent &event) {
  Reconstructor::operator()(event);
  if (telescopes.size() < 2) {
    energy_reco.energy_valid = false;
    event.dl2->energy[this->name()] = energy_reco;
    return;
  }
  if (!event.dl2->geometry.contains("HillasReconstructor")) {
    spdlog::error("HillasReconsturctor is not avaliable, it's better to have "
                  "it as initial value for LookupTableEnergyRegressor");
    throw std::runtime_error(
        "HillasReconsturctor is not avaliable, it's better to have it as "
        "initial value for LookupTableEnergyRegressor");
  }
  Eigen::VectorXd intensity = Eigen::VectorXd::Zero(telescopes.size());
  Eigen::VectorXd impact_parameters = Eigen::VectorXd::Zero(telescopes.size());
  for (int i = 0; i < telescopes.size(); i++) {
    if (use_fake_hillas) {
      intensity[i] = event.simulation->tels[telescopes[i]]
                         ->image_parameters.hillas.intensity;
    } else {
      intensity[i] =
          event.dl1->tels[telescopes[i]]->image_parameters.hillas.intensity;
    }
    impact_parameters[i] =
        event.dl2->tels[telescopes[i]].impact("HillasReconstructor").distance;
  }
  Eigen::VectorXd weights = Eigen::VectorXd::Zero(telescopes.size());
  Eigen::VectorXd tel_energies = Eigen::VectorXd::Zero(telescopes.size());
  std::vector<int> valid_telescopes;
  for (int i = 0; i < telescopes.size(); i++) {

    int tel_id = telescopes[i];
    InterpResult result = interpolate_histogram(
        mean_lookup_table, impact_parameters[i], log10(intensity[i]), -999);
    InterpResult sigma_result;
    if (use_sigma_as_weight) {
      sigma_result = interpolate_histogram(
          sigma_lookup_table, impact_parameters[i], log10(intensity[i]), -999);
      weights(i) = 1.0 / pow(sigma_result.value, 2);
    } else {
      sigma_result.inside = true;
      weights(i) = intensity[i];
    }
    if (!result.inside || !sigma_result.inside) {
      tel_energies(i) = 0;
      weights(i) = 0;
      continue;
    }
    tel_energies(i) = result.value;
    valid_telescopes.push_back(tel_id);
    event.dl2->set_tel_estimate_energy(tel_id, pow(10, tel_energies(i)));
  }
  energy_reco.telescopes = valid_telescopes;
  energy_reco.estimate_energy =
      pow(10, (tel_energies.array() * weights.array()).sum() / weights.sum());
  energy_reco.energy_valid = true;
  event.dl2->add_energy(this->name(), energy_reco);
}