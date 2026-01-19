/**
 * @file Convert.cpp
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Convert the RootEventSource to a much simplified version
 * @version 0.1
 * @date 2025-03-30
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "CoordFrames.hh"
#include "RootEventSource.hh"
#include "RootWriter.hh"
#include "Simplied_RootSource.hpp"
#include "TH1F.h"
#include "TH2F.h"
#include "Utils.hh"
#include "args.hxx"
#include <iostream>
#include <string>
#include <vector>
int main(int argc, const char *argv[]) {
  args::ArgumentParser parser(
      "Convert RootEventSource files to simplified format");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  // Define arguments that can be used multiple times
  args::ValueFlagList<std::string> input_files(
      parser, "input", "Input file (can be specified multiple times)",
      {'i', "input"});
  args::ValueFlagList<std::string> output_files(
      parser, "output", "Output file (can be specified multiple times)",
      {'o', "output"});

  try {
    parser.ParseCLI(argc, argv);
  } catch (const args::Help &) {
    std::cout << parser;
    return 0;
  } catch (const args::ParseError &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  } catch (const args::ValidationError &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  // Get the input and output files
  const std::vector<std::string> &inputs = input_files.Get();
  const std::vector<std::string> &outputs = output_files.Get();

  if (inputs.empty()) {
    std::cerr << "Error: At least one input file must be specified"
              << std::endl;
    return 1;
  }

  if (inputs.size() != outputs.size()) {
    std::cerr << "Error: Number of input files (" << inputs.size()
              << ") must match number of output files (" << outputs.size()
              << ")" << std::endl;
    return 1;
  }

  // Process each input-output pair
  for (size_t i = 0; i < inputs.size(); i++) {
    const std::string &input_file = inputs[i];
    const std::string &output_file = outputs[i];

    std::cout << "Converting " << input_file << " to " << output_file
              << std::endl;

    try {
      // Open input file
      std::unique_ptr<RootEventSource> source =
          std::make_unique<RootEventSource>(input_file, -1, std::vector<int>{},
                                            false);
      std::unique_ptr<TFile> output_root =
          std::unique_ptr<TFile>(TFile::Open(output_file.c_str(), "RECREATE"));
      TTree *teltree = new TTree("tels", "Telescope TTree data");
      TTree *eventtree = new TTree("events", "Event TTree data");
      TelescopeData data;
      EventData event_data;
      initialize_telescope_tree(teltree, data);
      initialize_event_tree(eventtree, event_data);
      auto subarray = source->subarray;

      // Rest of your processing code remains the same
      for (const auto &event : *source) {
        if (!event.dl2->geometry.at("HillasReconstructor").is_valid) {
          continue;
        }
        event_data.event_id = event.event_id;
        event_data.hillas_n_tels =
            event.dl2->geometry.at("HillasReconstructor").telescopes.size();
        std::vector<double> current_event_psis;
        current_event_psis.reserve(
            event.dl2->geometry.at("HillasReconstructor").telescopes.size());

        // 2. 收集 Psi 值
        for (const auto &tel_id :
             event.dl2->geometry.at("HillasReconstructor").telescopes) {
          // 注意：确保这里取出的 psi 是弧度 (radians)。Sim_telarray/ctapipe
          // 通常默认为弧度。 如果是角度，请先转换： psi * (M_PI / 180.0)
          double psi =
              event.simulation->tels.at(tel_id)->image_parameters.hillas.psi;
          current_event_psis.push_back(psi);
        }

        // 3. 计算 Geometric Weight (利用两两交叉)
        double sum_sin2_theta = 0.0;
        int n_pairs = 0; // 如果你需要计算归一化的 GW，可以用这个计数

        size_t n_tels = current_event_psis.size();

        if (n_tels >= 2) {
          // 双重循环遍历所有唯一的配对 (i < j)
          for (size_t i = 0; i < n_tels; ++i) {
            for (size_t j = i + 1; j < n_tels; ++j) {

              // 两台望远镜长轴夹角就是 psi 之差
              double delta_psi = current_event_psis[i] - current_event_psis[j];

              // 计算 sin^2(夹角)
              double sin_val = std::sin(delta_psi);
              sum_sin2_theta += (sin_val * sin_val);

              n_pairs++;
            }
          }
          // GW 定义为平方和的开根号
          event_data.gw = std::sqrt(sum_sin2_theta);
        } else {
          // 如果只有1台或0台望远镜，无法构成立体几何
          event_data.gw = 0.0;
        }
        event_data.hillas_rec_alt =
            event.dl2->geometry.at("HillasReconstructor").alt;
        event_data.hillas_rec_az =
            event.dl2->geometry.at("HillasReconstructor").az;
        event_data.hillas_rec_core_x =
            event.dl2->geometry.at("HillasReconstructor").core_x;
        event_data.hillas_rec_core_y =
            event.dl2->geometry.at("HillasReconstructor").core_y;
        event_data.hillas_direction_error =
            event.dl2->geometry.at("HillasReconstructor").direction_error;
        double sigma =
            pow(event.dl2->geometry.at("HillasReconstructor").alt_uncertainty,
                2) +
            pow(event.dl2->geometry.at("HillasReconstructor").az_uncertainty,
                2);
        event_data.hillas_direction_sigma = sqrt(sigma);
        event_data.shower = event.simulation->shower;
        event_data.hillas_hmax =
            event.dl2->geometry.at("HillasReconstructor").hmax;
        if (!event.dl2->particle.empty())
          event_data.hadroness =
              event.dl2->particle.at("MLParticleClassifier").hadroness;
        event_data.pointing_alt = event.pointing->array_altitude;
        event_data.pointing_az = event.pointing->array_azimuth;
        if (event.dl2->geometry.contains("HillasSumWeightedReconstructor")) {
          event_data.weighted_summed_rec_alt =
              event.dl2->geometry.at("HillasSumWeightedReconstructor").alt;
          event_data.weighted_sum_rec_az =
              event.dl2->geometry.at("HillasSumWeightedReconstructor").az;
          event_data.weighted_sum_direction_error =
              event.dl2->geometry.at("HillasSumWeightedReconstructor")
                  .direction_error;
          event_data.weighted_sum_direction_sigma =
              event.dl2->geometry.at("HillasSumWeightedReconstructor")
                  .alt_uncertainty;
        }
        if (event.dl2->geometry.contains("DispWeightedReconstructor")) {
          event_data.disp_rec_alt =
              event.dl2->geometry.at("DispWeightedReconstructor").alt;
          event_data.disp_rec_az =
              event.dl2->geometry.at("DispWeightedReconstructor").az;
          event_data.disp_direction_error =
              event.dl2->geometry.at("DispWeightedReconstructor")
                  .direction_error;
          event_data.disp_direction_sigma =
              event.dl2->geometry.at("DispWeightedReconstructor")
                  .alt_uncertainty;
        }
        if (event.dl2->geometry.contains("TestReconstructor")) {
          event_data.test_rec_alt =
              event.dl2->geometry.at("TestReconstructor").alt;
          event_data.test_rec_az =
              event.dl2->geometry.at("TestReconstructor").az;
          event_data.test_direction_error =
              event.dl2->geometry.at("TestReconstructor").direction_error;
          event_data.test_rec_core_x =
              event.dl2->geometry.at("TestReconstructor").core_x;
          event_data.test_rec_core_y =
              event.dl2->geometry.at("TestReconstructor").core_y;
        }
        if (!event.dl2->energy.empty()) {
          event_data.rec_energy =
              event.dl2->energy.at("EnergyRegressor").estimate_energy;
          if (event.dl2->energy.contains("TestReconstructor_energy") &&
              event.dl2->energy.at("TestReconstructor_energy").energy_valid) {
            event_data.flow_rec_energy =
                event.dl2->energy.at("TestReconstructor_energy")
                    .estimate_energy;
          } else {
            event_data.flow_rec_energy = 0;
          }
        } else {
          event_data.rec_energy = 0;
        }
        eventtree->Fill();
        double average_intensity_sum = 0;
        double average_intensity =
            average_intensity_sum / event.dl2->tels.size();
        for (const auto &tel_id :
             event.dl2->geometry.at("HillasReconstructor").telescopes) {
          if (!event.simulation->tels.contains(tel_id))
            continue;
          auto tel_coord = subarray->tel_positions.at(tel_id);
          double impact_parameter =
              event.simulation->tels.at(tel_id)->impact_parameter;
          data.event_id = event.event_id;
          data.tel_id = tel_id;
          if (!event.dl2->tels.at(tel_id).impact_parameters.contains(
                  "HillasReconstructor")) {
            throw std::runtime_error(
                "HillasReconstructor impact parameter not found");
          }
          data.rec_impact_parameter =
              event.dl2->tels.at(tel_id)
                  .impact_parameters.at("HillasReconstructor")
                  .distance;
          data.true_impact_parameter = impact_parameter;
          // data.params = event.dl1->tels.at(tel_id)->image_parameters;
          data.fake_params =
              event.simulation->tels.at(tel_id)->image_parameters;
          data.true_alt = event.simulation->shower.alt;
          data.true_az = event.simulation->shower.az;
          data.true_energy = event.simulation->shower.energy;
          data.rec_alt = event.dl2->geometry.at("HillasReconstructor").alt;
          data.rec_az = event.dl2->geometry.at("HillasReconstructor").az;
          data.average_intensity = average_intensity;
          data.hillas_hmax = event.dl2->geometry.at("HillasReconstructor").hmax;
          if (!event.dl2->energy.empty()) {
            data.rec_energy =
                event.dl2->energy.at("EnergyRegressor").estimate_energy;
            data.tel_rec_energy = event.dl2->tels.at(tel_id).estimate_energy;
          } else {
            data.rec_energy = 0;
            data.tel_rec_energy = 0;
          }
          data.tel_rec_disp = event.dl2->tels.at(tel_id).estimate_disp;
          data.n_tel =
              event.dl2->geometry.at("HillasReconstructor").telescopes.size();
          teltree->Fill();
        }
      }

      if (source->statistics.has_value()) {
        int ihist = 0;
        auto statistics = source->statistics.value();
        for (const auto &[name, hist] : statistics.histograms) {
          if (hist->get_dimension() == 1) {
            auto h1d = dynamic_cast<Histogram1D<float> *>(hist.get());
            auto new_hist = new TH1F(("h" + std::to_string(ihist)).c_str(),
                                     name.c_str(), h1d->bins(),
                                     h1d->get_low_edge(), h1d->get_high_edge());
            for (int i = 0; i < h1d->bins(); i++) {
              new_hist->SetBinContent(i + 1, h1d->get_bin_content(i));
            }
            new_hist->Write();
            ihist++;
          } else if (hist->get_dimension() == 2) {
            auto h2d = dynamic_cast<Histogram2D<float> *>(hist.get());
            auto new_hist = new TH2F(
                ("h" + std::to_string(ihist)).c_str(), name.c_str(),
                h2d->x_bins(), h2d->get_x_low_edge(), h2d->get_x_high_edge(),
                h2d->y_bins(), h2d->get_y_low_edge(), h2d->get_y_high_edge());
            for (int i = 0; i < h2d->x_bins(); i++) {
              for (int j = 0; j < h2d->y_bins(); j++) {
                new_hist->SetBinContent(i + 1, j + 1, h2d->operator()(i, j));
              }
            }
            new_hist->Write();
            ihist++;
          }
        }
      }
      output_root->Write();
    } catch (const std::exception &e) {
      std::cerr << "Error processing " << input_file << ": " << e.what()
                << std::endl;
      continue; // Continue with next file instead of stopping
    }
  }

  return 0;
}