/**
 * @file LookupTableParticleClassifier.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Classical Method based on lookuptable to classify the particle type
 *        Owing that it dependes on the ROOT TH2D, so we put it in the root
 *        directory. It may be better to locate in the main directory by
 *        removing the dependency on ROOT.
 * @version 0.1
 * @date 2026-02-02
 *
 * @copyright Copyright (c) 2025
 *
 */
 #pragma once
 #include "ReconstructedGeometry.hh"
 #include "Reconstructor.hh"
 #include "TFile.h"
 #include "TH2D.h"
 /**
  * @brief LookupTableEnergyRegressor relies on the following structure of
  * lookuptable:
  * Two TH2D objects:
  * - mean_energy_regression
  * - sigma_energy_regression: if not provided, intensity will be used as weight
  *
  * The x-axis is impact parameter, the y-axis is log10(intensity).
  */
 class LookupTableParticleClassifier : public Reconstructor {
 public:
   LookupTableParticleClassifier(const json &config) : Reconstructor(config) {
     if (!config.contains("LookupTablePath")) {
       throw std::runtime_error("LookupTablePath is not set");
     }
     load_lookup_table(config["LookupTablePath"].get<std::string>());
   }
   LookupTableParticleClassifier(const std::string &config_str)
       : Reconstructor(config_str) {
     json config = json::parse(config_str);
     if (!config.contains("LookupTablePath")) {
       throw std::runtime_error("LookupTablePath is not set");
     }
     load_lookup_table(config["LookupTablePath"].get<std::string>());
   }
   ~LookupTableParticleClassifier() = default;
   void operator()(ArrayEvent &event) override;
   void load_lookup_table(const std::string &lookup_table_path);
   std::string name() const override { return "LookupTableParticleClassifier"; }
   ReconstructedParticle particle_reco;
 
 private:
   std::unique_ptr<TFile> lookup_table_file;
   TH2D *mean_length_lookup_table;
   TH2D *sigma_length_lookup_table;
   TH2D *mean_width_lookup_table;
   TH2D *sigma_width_lookup_table;
 
 };