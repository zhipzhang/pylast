#include "LookupTableEnergyRegressor.hh"
#include "ReconstructorFactory.hh"
#include "TFile.h"
#include "TH2D.h"
struct InterpResult {
  double value;
  bool inside;       // 是否在直方图范围内
  bool bilinear;     // 是否满足完整2x2（不在边界）
  bool neighbors_ok; // 4个角点都有效
};

InterpResult InterpWithQA(const TH2 *h2, double x, double y) {
  InterpResult r;
  auto ax = h2->GetXaxis();
  auto ay = h2->GetYaxis();

  r.inside = (x >= ax->GetXmin() && x <= ax->GetXmax() && y >= ay->GetXmin() &&
              y <= ay->GetXmax());

  int bx = ax->FindFixBin(x);
  int by = ay->FindFixBin(y);

  r.bilinear =
      (bx >= 1 && bx < ax->GetNbins()) && (by >= 1 && by < ay->GetNbins());
  // 这里的条件保证 bx+1 / by+1 不越界（能形成2×2）
  // 如果你想更严格避免贴边，可改成 bx>=2 && bx<=Nbins-1 等

  auto valid = [](double v) { return std::isfinite(v) && v > 0; };

  double z11 = h2->GetBinContent(bx, by);
  double z21 = h2->GetBinContent(bx + 1, by);
  double z12 = h2->GetBinContent(bx, by + 1);
  double z22 = h2->GetBinContent(bx + 1, by + 1);

  r.neighbors_ok = valid(z11) && valid(z21) && valid(z12) && valid(z22);

  r.value = h2->Interpolate(x, y);
  return r;
}

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
  for (int i = 0; i < telescopes.size(); i++) {
    int tel_id = telescopes[i];
    InterpResult result = InterpWithQA(mean_lookup_table, impact_parameters[i],
                                       log10(intensity[i]));
    InterpResult sigma_result;
    if (use_sigma_as_weight) {
      sigma_result = InterpWithQA(sigma_lookup_table, impact_parameters[i],
                                  log10(intensity[i]));
      weights(i) = 1.0 / sigma_result.value;
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
    event.dl2->set_tel_estimate_energy(tel_id, tel_energies(i));
  }
  energy_reco.estimate_energy =
      (tel_energies.array() * weights.array()).sum() / weights.sum();
  energy_reco.energy_valid = true;
  event.dl2->add_energy(this->name(), energy_reco);
}