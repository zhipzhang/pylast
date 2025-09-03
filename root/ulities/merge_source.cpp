/**
 * @file merge_source.cpp
 * @brief Merge multiple ROOT input files into a single output using DataWriter
 */

#include "args.hxx"
#include "RootEventSource.hh"
#include "DataWriter.hh"
#include "spdlog/spdlog.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

int main(int argc, const char* argv[])
{
    args::ArgumentParser parser("Merge multiple ROOT event files into one output file");
    args::HelpFlag help(parser, "help", "Show help", {'h', "help"});
    args::ValueFlagList<std::string> inputs(parser, "input", "Input ROOT file (-i/--input, repeatable)", {'i', "input"});
    args::ValueFlag<std::string> output(parser, "output", "Output ROOT file (-o/--output)", {'o', "output"});
    args::ValueFlag<std::string> config(parser, "config", "JSON configuration file (-c/--config)", {'c', "config"});

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

    const auto in_files = inputs.Get();
    const auto out_file = output.Get();
    const auto config_file = config.Get();

    if (in_files.empty()) {
        std::cerr << "Error: At least one -i/--input is required" << std::endl;
        return 1;
    }
    if (!output) {
        std::cerr << "Error: -o/--output is required" << std::endl;
        return 1;
    }

    try {
        // Open first source to initialize writer and static metadata (subarray, configs, etc.)
        auto first_source = std::make_unique<RootEventSource>(in_files.front(), -1, std::vector<int>{}, true);

        // Configure DataWriter once; it will own and keep the output file open
        DataWriter writer(*first_source, out_file);
        
        // Load configuration
        json cfg;
        if (config) {
            // Read external JSON config file
            std::ifstream config_stream(config_file);
            if (!config_stream.is_open()) {
                throw std::runtime_error("Cannot open config file: " + config_file);
            }
            config_stream >> cfg;
            spdlog::info("Loaded configuration from {}", config_file);
        } else {
            // Use default configuration
            cfg = DataWriter::get_default_config();
        }
        
        // Ensure we recreate the output file
        cfg["overwrite"] = true;
        cfg["write_simulated_camera"] = true; // No simulation camera data in input files
        writer.configure(cfg);

        // Write events from first source
        writer.write_all_simulation_shower(first_source->get_shower_array());
        writer.write_statistics(first_source->statistics.value(), false);
        for (const auto& ev : *first_source) {
            writer(ev);
        }

        // Process remaining sources: reuse the same writer, just feed events
        for (size_t i = 1; i < in_files.size(); ++i) {
            spdlog::info("Merging from {}", in_files[i]);
            auto src = std::make_unique<RootEventSource>(in_files[i], -1, std::vector<int>{}, false);
            writer.write_all_simulation_shower(src->get_shower_array());
            writer.write_statistics(src->statistics.value(), false);
            for (const auto& ev : *src) {
                writer(ev);
            }
        }
        // Explicitly close to flush and build indices
        writer.close();
        spdlog::info("Merged {} file(s) into {}", in_files.size(), out_file);
    } catch (const std::exception& e) {
        spdlog::error("Merge failed: {}", e.what());
        return 2;
    }

    return 0;
}
