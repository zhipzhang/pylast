/**
 * @file Simplified_Convert.cpp
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Convert RootEventSource files into a simplified format
 * @version 0.1
 * @date 2025-03-30
 * @copyright Copyright (c) 2025
 */

#include "CoordFrames.hh"
#include "RootEventSource.hh"
#include "RootWriter.hh"
#include "Simplied_RootSource.hpp"
#include "TH1F.h"
#include "TH2F.h"
#include "Utils.hh"
#include "args.hxx"
#include <spdlog/spdlog.h>
#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <cmath>

void print_argument_error(const std::string &msg, const args::ArgumentParser &parser) {
    std::cerr << msg << std::endl << parser;
}

bool parse_arguments(int argc, const char *argv[],
                     std::vector<std::string> &inputs,
                     std::vector<std::string> &outputs) {
    args::ArgumentParser parser("Convert RootEventSource files to simplified format");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlagList<std::string> input_files(parser, "input", "Input file (can be specified multiple times)", {'i', "input"});
    args::ValueFlagList<std::string> output_files(parser, "output", "Output file (can be specified multiple times)", {'o', "output"});

    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Help &) {
        std::cout << parser;
        return false;
    } catch (const args::ParseError &e) {
        print_argument_error(e.what(), parser);
        return false;
    } catch (const args::ValidationError &e) {
        print_argument_error(e.what(), parser);
        return false;
    }

    inputs = input_files.Get();
    outputs = output_files.Get();

    if (inputs.empty()) {
        std::cerr << "Error: At least one input file must be specified." << std::endl;
        return false;
    }
    if (inputs.size() != outputs.size()) {
        std::cerr << "Error: Number of input files (" << inputs.size()
                  << ") does not match number of output files (" << outputs.size() << ")." << std::endl;
        return false;
    }
    return true;
}

void set_event_data_basic(EventData &event_data, const auto &event) {
    const auto &hillas = event.dl2->geometry.at("HillasReconstructor");
    event_data.event_id      = event.event_id;
    event_data.hillas_n_tels = hillas.telescopes.size();
    event_data.hillas_rec_alt = hillas.alt;
    event_data.hillas_rec_az = hillas.az;
    event_data.hillas_rec_core_x = hillas.core_x;
    event_data.hillas_rec_core_y = hillas.core_y;
    event_data.hillas_direction_error = hillas.direction_error;
    double sigma = std::pow(hillas.alt_uncertainty, 2) + std::pow(hillas.az_uncertainty, 2);
    event_data.hillas_direction_sigma = std::sqrt(sigma);
    event_data.hillas_hmax = hillas.hmax;
    event_data.shower = event.simulation->shower;
}

void set_event_data_classification(EventData &event_data, const auto &event) {
    // ParticleClassifier (Hadroness)
    if (!event.dl2->particle.empty()
        && event.dl2->particle.contains("ParticleClassifier")
        && event.dl2->particle.at("ParticleClassifier").is_valid) {
        event_data.hadroness = event.dl2->particle.at("ParticleClassifier").hadroness;
    } else {
        event_data.hadroness = -999;
    }
    // LookupTable ParticleClassifier (MRSL/MRSW)
    if (!event.dl2->particle.empty()
        && event.dl2->particle.contains("LookupTableParticleClassifier")
        && event.dl2->particle.at("LookupTableParticleClassifier").is_valid) {
        event_data.mrsl = event.dl2->particle.at("LookupTableParticleClassifier").mrsl;
        event_data.mrsw = event.dl2->particle.at("LookupTableParticleClassifier").mrsw;
    } else {
        event_data.mrsl = -999;
        event_data.mrsw = -999;
    }
}

void set_event_data_pointing(EventData &event_data, const auto &event) {
    event_data.pointing_alt = event.pointing->array_altitude;
    event_data.pointing_az  = event.pointing->array_azimuth;
}

void set_event_data_optional_geometries(EventData &event_data, const auto &event) {
    // HillasSumWeightedReconstructor
    if (event.dl2->geometry.contains("HillasSumWeightedReconstructor")) {
        const auto &sum = event.dl2->geometry.at("HillasSumWeightedReconstructor");
        event_data.weighted_summed_rec_alt   = sum.alt;
        event_data.weighted_sum_rec_az       = sum.az;
        event_data.weighted_sum_direction_error = sum.direction_error;
        event_data.weighted_sum_direction_sigma = sum.alt_uncertainty;
    }
    // DispWeightedReconstructor
    if (event.dl2->geometry.contains("DispWeightedReconstructor")) {
        const auto &disp = event.dl2->geometry.at("DispWeightedReconstructor");
        event_data.disp_rec_alt      = disp.alt;
        event_data.disp_rec_az       = disp.az;
        event_data.disp_direction_error = disp.direction_error;
        event_data.disp_direction_sigma = disp.alt_uncertainty;
    }
    // TestReconstructor
    if (event.dl2->geometry.contains("TestReconstructor")) {
        const auto &test = event.dl2->geometry.at("TestReconstructor");
        event_data.test_rec_alt  = test.alt;
        event_data.test_rec_az   = test.az;
        event_data.test_direction_error = test.direction_error;
        event_data.test_rec_core_x = test.core_x;
        event_data.test_rec_core_y = test.core_y;
    }
}

void set_event_data_energy(EventData &event_data, const auto &event) {
    // Default values
    event_data.rec_energy = 0;
    event_data.rec_energy_std = 0;
    event_data.flow_rec_energy = 0;

    if (event.dl2->energy.empty()) return;

    if (!event.dl2->energy.contains("EnergyRegressor")) {
        spdlog::error("EnergyRegressor not found in event.dl2->energy for event {}", event.event_id);
        return;
    }
    // Main energy
    event_data.rec_energy = event.dl2->energy.at("EnergyRegressor").estimate_energy;

    // Energy spread per telescope (stddev)
    std::vector<double> energies, energies_sq;
    const auto &hillas = event.dl2->geometry.at("HillasReconstructor");
    for (const auto &tel_id : hillas.telescopes) {
        if (!event.dl2->tels.contains(tel_id)) {
            spdlog::warn("Telescope {} not found in event.dl2->tels for event {}", tel_id, event.event_id);
            continue;
        }
        double e = std::pow(10, event.dl2->tels.at(tel_id).estimate_energy);
        energies.push_back(e);
        energies_sq.push_back(e * e);
    }
    if (!energies.empty()) {
        double mean = std::accumulate(energies.begin(), energies.end(), 0.0) / energies.size();
        double mean_sq = std::accumulate(energies_sq.begin(), energies_sq.end(), 0.0) / energies_sq.size();
        event_data.rec_energy_std = std::sqrt(mean_sq - mean * mean);
    }

    // Optional: TestReconstructor_energy
    if (event.dl2->energy.contains("TestReconstructor_energy")
        && event.dl2->energy.at("TestReconstructor_energy").energy_valid) {
        event_data.flow_rec_energy = event.dl2->energy.at("TestReconstructor_energy").estimate_energy;
    }
}

void process_telescope_data(const auto &event, TelescopeData &tel_data, EventData &event_data, TTree *teltree) {
    for (const auto &[tel_id, tel_obj] : event.dl2->tels) {
        tel_data.event_id = event.event_id;
        tel_data.tel_id   = tel_id;
        tel_data.true_impact_parameter = event.simulation->tels.at(tel_id)->impact_parameter;
        tel_data.true_alt      = event.simulation->shower.alt;
        tel_data.true_az       = event.simulation->shower.az;
        tel_data.true_energy   = event.simulation->shower.energy;

        tel_data.fake_params = event.simulation->tels.at(tel_id)->image_parameters;
        tel_data.rec_impact_parameter = tel_obj.impact_parameters.at("HillasReconstructor").distance;
        // Reconstructed geometry
        tel_data.rec_alt = event.dl2->geometry.at("HillasReconstructor").alt;
        tel_data.rec_az  = event.dl2->geometry.at("HillasReconstructor").az;
        tel_data.hillas_hmax = event.dl2->geometry.at("HillasReconstructor").hmax;
        tel_data.n_tel = event.dl2->geometry.at("HillasReconstructor").telescopes.size();

        // Energy reconstruction
        if (!event.dl2->energy.empty() &&
            event.dl2->energy.contains("EnergyRegressor")) {
            tel_data.rec_energy = event.dl2->energy.at("EnergyRegressor").estimate_energy;
        } else {
            tel_data.rec_energy = 0;
        }
        tel_data.tel_rec_energy      = tel_obj.estimate_energy;
        tel_data.tel_rec_energy_std  = event_data.rec_energy_std;
        tel_data.tel_rec_hadroness   = tel_obj.estimate_hadroness;
        tel_data.tel_rec_disp        = tel_obj.estimate_disp;
        teltree->Fill();
    }
}

void write_histograms(const RootEventSource &source, TFile *output_root) {
    if (!source.statistics.has_value()) return;
    int ihist = 0;
    auto &statistics = source.statistics.value();
    for (const auto &[name, hist] : statistics.histograms) {
        if (hist->get_dimension() == 1) {
            auto h1d = dynamic_cast<Histogram1D<float> *>(hist.get());
            auto new_hist = new TH1F(
                ("h" + std::to_string(ihist)).c_str(),
                name.c_str(),
                h1d->bins(),
                h1d->get_low_edge(),
                h1d->get_high_edge()
            );
            for (int i = 0; i < h1d->bins(); ++i)
                new_hist->SetBinContent(i + 1, h1d->get_bin_content(i));
            new_hist->Write();
            ++ihist;
        } else if (hist->get_dimension() == 2) {
            auto h2d = dynamic_cast<Histogram2D<float> *>(hist.get());
            auto new_hist = new TH2F(
                ("h" + std::to_string(ihist)).c_str(),
                name.c_str(),
                h2d->x_bins(), h2d->get_x_low_edge(), h2d->get_x_high_edge(),
                h2d->y_bins(), h2d->get_y_low_edge(), h2d->get_y_high_edge()
            );
            for (int i = 0; i < h2d->x_bins(); ++i)
                for (int j = 0; j < h2d->y_bins(); ++j)
                    new_hist->SetBinContent(i + 1, j + 1, (*h2d)(i, j));
            new_hist->Write();
            ++ihist;
        }
    }
}

int main(int argc, const char *argv[]) {
    std::vector<std::string> inputs, outputs;
    if (!parse_arguments(argc, argv, inputs, outputs))
        return 1;

    for (size_t i = 0; i < inputs.size(); ++i) {
        const std::string &input_file = inputs[i];
        const std::string &output_file = outputs[i];

        std::cout << "Converting " << input_file << " to " << output_file << std::endl;

        try {
            // Load input ROOT file
            std::unique_ptr<RootEventSource> source = std::make_unique<RootEventSource>(
                input_file, -1, std::vector<int>{}, false
            );
            std::unique_ptr<TFile> output_root(TFile::Open(output_file.c_str(), "RECREATE"));

            // Prepare output trees and data structures
            TTree *teltree   = new TTree("tels", "Telescope TTree data");
            TTree *eventtree = new TTree("events", "Event TTree data");
            TelescopeData teldata;
            EventData event_data;
            initialize_telescope_tree(teltree, teldata);
            initialize_event_tree(eventtree, event_data);
            // auto subarray = source->subarray; // (Unused here)

            // Loop over all events in source
            for (const auto &event : *source) {
                // -- Only process events with valid HillasReconstructor geometry --
                if (!event.dl2->geometry.contains("HillasReconstructor"))
                    continue;
                if (!event.dl2->geometry.at("HillasReconstructor").is_valid)
                    continue;

                // Set main event summary data
                set_event_data_basic(event_data, event);
                set_event_data_classification(event_data, event);
                set_event_data_pointing(event_data, event);
                set_event_data_optional_geometries(event_data, event);
                set_event_data_energy(event_data, event);

                eventtree->Fill();
                process_telescope_data(event, teldata, event_data, teltree);
            }
            // Write any histograms if present
            write_histograms(*source, output_root.get());
            output_root->Write();
        } catch (const std::exception &e) {
            std::cerr << "Error processing " << input_file << ": " << e.what() << std::endl;
            continue;
        }
    }
    return 0;
}
