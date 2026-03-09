#include "EnergyRegressor.hh"
#include "ReconstructorFactory.hh"

static bool registered_energy_regressor = []() {
  ReconstructorFactory::instance().register_reconstructor(
      "EnergyRegressor",
      [](const SubarrayDescription &subarray,
         const json &config) -> std::unique_ptr<Reconstructor> {
        return std::make_unique<EnergyRegressor>(config);
      });
  return true;
}();

void EnergyRegressor::operator()(ArrayEvent &event) {
  Reconstructor::operator()(event);
  if (telescopes.size() < 2) {
    energy_reco.energy_valid = false;
    event.dl2->energy[this->name()] = energy_reco;
    return;
  }
  if (!event.dl2->geometry.contains("HillasReconstructor")) {
    spdlog::error("HillasReconsturctor is not avaliable, it's better to have "
                  "it as initial value for EnergyRegressor");
    throw std::runtime_error(
        "HillasReconsturctor is not avaliable, it's better to have it as "
        "initial value for EnergyRegressor");
  }
  double rec_alt = event.dl2->geometry["HillasReconstructor"].alt;
  double rec_az = event.dl2->geometry["HillasReconstructor"].az;
  double rec_offset = compute_angle_separation(
                          rec_az, rec_alt, array_pointing_direction.azimuth,
                          array_pointing_direction.altitude) *
                      180 / M_PI;

  Eigen::VectorXd weights = Eigen::VectorXd::Zero(telescopes.size());
  Eigen::VectorXd tel_energies = Eigen::VectorXd::Zero(telescopes.size());
  for (int i = 0; i < telescopes.size(); i++) {
    int tel_id = telescopes[i];
    double tel_energy;
    tel_energy = energy_offset_estimator->predict(rec_offset, event, tel_id);
    tel_energies(i) = pow(10, tel_energy);
    weights(i) =
        event.simulation->tels[tel_id]->image_parameters.hillas.intensity;
    event.dl2->set_tel_estimate_energy(tel_id, tel_energy);
  }
  energy_reco.estimate_energy =
      (tel_energies.array() * weights.array()).sum() / weights.sum();
  energy_reco.energy_valid = true;
  energy_reco.estimate_energy_std = sqrt(tel_energies.array().square().sum()/ tel_energies.array().size() - pow(tel_energies.array().mean(), 2));
  energy_reco.telescopes = telescopes;
  event.dl2->add_energy(this->name(), energy_reco);
}