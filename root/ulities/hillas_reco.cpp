/**
 * @file hillas_reco.cpp
 * @author Your Name
 * @brief Native C++ implementation of hillas_reco command
 * @version 0.1
 * @date 2025-09-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "SimtelEventSource.hh"
#include "Calibration.hh"
#include "ImageProcessor.hh"
#include "ShowerProcessor.hh"
#include "DataWriter.hh"
#include "Statistics.hh"
#include "RootWriter.hh"
#include "histogram.hpp"
#include "args.hxx"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <exception>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace eigen_histogram;

// Default configuration function
json default_config() {
    json config;
    
    // Create empty dictionaries for various configuration parameters
    config["calibrator"] = json::object();
    config["image_processor"] = json::object();
    config["shower_processor"] = json::object();
    config["data_writer"] = json::object();
    
    // Calibration defaults
    config["calibrator"]["image_extractor_type"] = "LocalPeakExtractor";
    config["calibrator"]["LocalPeakExtractor"] = json::object();
    config["calibrator"]["LocalPeakExtractor"]["window_shift"] = 3;
    config["calibrator"]["LocalPeakExtractor"]["window_width"] = 7;
    config["calibrator"]["LocalPeakExtractor"]["apply_correction"] = true;
    
    // Image cleaning defaults
    config["image_processor"]["poisson_noise"] = 5;
    config["image_processor"]["image_cleaner_type"] = "Tailcuts_cleaner";
    config["image_processor"]["TailcutsCleaner"] = json::object();
    config["image_processor"]["TailcutsCleaner"]["picture_thresh"] = 15.0;
    config["image_processor"]["TailcutsCleaner"]["boundary_thresh"] = 7.5;
    config["image_processor"]["TailcutsCleaner"]["keep_isolated_pixels"] = false;
    config["image_processor"]["TailcutsCleaner"]["min_number_picture_neighbors"] = 2;
    
    // Shower reconstruction defaults
    config["shower_processor"]["GeometryReconstructionTypes"] = std::vector<std::string>{"HillasReconstructor"};
    
    // Image Cuts
    config["shower_processor"]["MLParticleClassifier"] = json::object();
    config["shower_processor"]["MLParticleClassifier"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3 && hillas_width > 0 && morphology_n_pixels >= 5";
    config["shower_processor"]["HillasReconstructor"] = json::object();
    config["shower_processor"]["HillasWeightedReconstructor"] = json::object();
    config["shower_processor"]["HillasReconstructor"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3 && hillas_width > 0 && morphology_n_pixels >= 5";
    config["shower_processor"]["HillasReconstructor"]["use_fake_hillas"] = true;
    config["shower_processor"]["HillasWeightedReconstructor"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3 && hillas_width > 0 && morphology_n_pixels >= 5";
    config["shower_processor"]["MLEnergyReconstructor"] = json::object();
    config["shower_processor"]["MLEnergyReconstructor"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3 && hillas_width > 0 && morphology_n_pixels >= 5";

    config["data_writer"]["output_type"] = "root";
    config["data_writer"]["eos_url"] = "root://eos01.ihep.ac.cn/";
    config["data_writer"]["overwrite"] = true;
    config["data_writer"]["write_simulation_shower"] = true;
    config["data_writer"]["write_simulated_camera"] = true;
    config["data_writer"]["write_simulated_camera_image"] = false;
    config["data_writer"]["write_r0"] = false;
    config["data_writer"]["write_r1"] = false;
    config["data_writer"]["write_dl0"] = false;
    config["data_writer"]["write_dl1"] = true;
    config["data_writer"]["write_dl1_image"] = true;
    config["data_writer"]["write_dl2"] = true;
    config["data_writer"]["write_monitor"] = false;
    config["data_writer"]["write_pointing"] = true;
    config["data_writer"]["write_simulation_config"] = false;
    config["data_writer"]["write_atmosphere_model"] = false;
    config["data_writer"]["write_subarray"] = true;
    config["data_writer"]["write_metaparam"] = false;
    
    return config;
}

int main(int argc, const char* argv[]) {
    args::ArgumentParser parser("Process multiple input files and save results to corresponding output files");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    
    // Define arguments that can be used multiple times
    args::ValueFlagList<std::string> input_files(parser, "input", "Input file path (can be specified multiple times)", {'i', "input"});
    args::ValueFlagList<std::string> output_files(parser, "output", "Output file path (can be specified multiple times)", {'o', "output"});
    args::ValueFlag<std::string> config_file(parser, "config", "Config file path (If not provided, default config will be used)", {'c', "config"});
    args::ValueFlag<std::string> max_leakage2(parser, "max_leakage2", "Max leakage2 for hillas reconstruction", {'l', "max-leakage2"});
    args::ValueFlag<std::string> subarray(parser, "subarray", "Specify telescopes to use (comma-separated list, e.g., '1,2,3,4')", {'s', "subarray"});
    
    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Help&) {
        std::cout << parser;
        return 0;
    } catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    } catch (const args::ValidationError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    
    // Get the input and output files
    const std::vector<std::string>& inputs = input_files.Get();
    const std::vector<std::string>& outputs = output_files.Get();
    
    if (inputs.empty()) {
        std::cerr << "Error: At least one input file must be specified" << std::endl;
        return 1;
    }
    
    if (inputs.size() != outputs.size()) {
        std::cerr << "Error: Number of input files (" << inputs.size() 
                  << ") must match number of output files (" << outputs.size() << ")" << std::endl;
        return 1;
    }
    
    // Load configuration
    json config;
    if (config_file) {
        std::ifstream f(config_file.Get());
        if (!f.is_open()) {
            std::cerr << "Error: Could not open config file: " << config_file.Get() << std::endl;
            return 1;
        }
        f >> config;
    } else {
        config = default_config();
    }
    
    // Update config with max_leakage2 if provided
    if (max_leakage2) {
        std::string leakage_query = "leakage_intensity_width_2 < " + max_leakage2.Get() + " && hillas_intensity > 100";
        config["shower_processor"]["HillasReconstructor"]["ImageQuery"] = leakage_query;
    }
    
    // Parse subarray parameter if provided
    std::vector<int> tel_ids;
    if (subarray) {
        std::string subarray_str = subarray.Get();
        size_t pos = 0;
        std::string token;
        while ((pos = subarray_str.find(',')) != std::string::npos) {
            token = subarray_str.substr(0, pos);
            try {
                tel_ids.push_back(std::stoi(token));
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid telescope ID in subarray list: " << token << std::endl;
                return 1;
            }
            subarray_str.erase(0, pos + 1);
        }
        // Add the last token
        try {
            tel_ids.push_back(std::stoi(subarray_str));
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid telescope ID in subarray list: " << subarray_str << std::endl;
            return 1;
        }
    }
    
    // Process each input-output pair
    for (size_t i = 0; i < inputs.size(); i++) {
        const std::string& input_file = inputs[i];
        const std::string& output_file = outputs[i];
        
        std::cout << "Processing " << input_file << " -> " << output_file << std::endl;
        
        try {
            // Create source
            std::unique_ptr<SimtelEventSource> source;
            if (!tel_ids.empty()) {
                source = std::make_unique<SimtelEventSource>(input_file, -1, tel_ids, false);
            } else {
                source = std::make_unique<SimtelEventSource>(input_file, -1, std::vector<int>{}, false);
            }
            
            // Initialize statistics histograms
            Statistics statistics;
            auto simulated_shower_hist = make_regular_histogram<float>(-1, 3, 60);
            auto direction_error_versus_energ_hist = make_regular_histogram_2d<float>(-1, 3, 60, 0, 1, 1000);
            
            // Initialize processors
            auto calibrator = std::make_unique<Calibrator>(*source->subarray, config["calibrator"]);
            auto image_processor = std::make_unique<ImageProcessor>(*source->subarray, config["image_processor"]);
            auto shower_processor = std::make_unique<ShowerProcessor>(*source->subarray, config["shower_processor"]);
            auto data_writer = std::make_unique<DataWriter>(*source, output_file, config["data_writer"]);

            // Process events
            for (auto& event : *source) {
                // Process the event
                (*calibrator)(event);
                (*image_processor)(event);
                (*shower_processor)(event);
                (*data_writer)(event);

                // Fill histograms for valid reconstructions
                if (event.dl2->geometry.contains("HillasReconstructor") && 
                    event.dl2->geometry.at("HillasReconstructor").is_valid) {
                    // Fill angular resolution vs energy histogram
                    float log_energy = std::log10(event.simulation->shower.energy);
                    float direction_error = event.dl2->geometry.at("HillasReconstructor").direction_error;
                    direction_error_versus_energ_hist.fill(log_energy, direction_error);
                }
            }
            
            // Add histograms to statistics
            statistics.add_histogram("Direction Error(deg) versus True Energy(TeV)", direction_error_versus_energ_hist);
            
            // Fill simulated shower histogram
            // Note: This requires accessing the shower array from the source
            for (const auto& energy : source->get_shower_array().energy()) {
                double log_energy = std::log10(energy);
                simulated_shower_hist.fill(log_energy);
            }
            
            // Add histogram to statistics
            statistics.add_histogram("log10(True Energy(TeV))", simulated_shower_hist);
            
            // Write statistics and simulation showers
            data_writer->write_statistics(statistics, true);
            data_writer->write_all_simulation_shower(source->get_shower_array());
            data_writer->close();
            std::cout << "Finished processing " << input_file << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error processing " << input_file << ": " << e.what() << std::endl;
            // Continue with next file instead of stopping
            continue;
        }
    }
    
    std::cout << "Processing complete" << std::endl;
    return 0;
}