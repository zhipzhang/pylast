/**
 * @file merge_source.cpp
 * @brief Merge multiple ROOT input files into a single output using SourceMerger.
 *        Supports multiple -i flags and wildcard patterns (e.g. -i "sim_*.root").
 *        Only ROOT files are accepted; SimtelEventSource is not supported.
 */

#include "SourceMerger.hh"
#include "TFile.h"
#include "args.hxx"
#include "spdlog/spdlog.h"
#include <glob.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<std::string> expand_glob(const std::string &pattern) {
  glob_t result;
  std::vector<std::string> files;
  int rc = glob(pattern.c_str(), GLOB_TILDE | GLOB_BRACE, nullptr, &result);
  if (rc == 0) {
    for (size_t i = 0; i < result.gl_pathc; ++i)
      files.emplace_back(result.gl_pathv[i]);
  } else if (rc != GLOB_NOMATCH) {
    globfree(&result);
    throw std::runtime_error("glob() failed for pattern: " + pattern);
  }
  globfree(&result);
  return files;
}

int main(int argc, const char *argv[]) {
  args::ArgumentParser parser(
      "Merge multiple ROOT event files into one output file");
  args::HelpFlag help(parser, "help", "Show help", {'h', "help"});
  args::ValueFlagList<std::string> inputs(
      parser, "input",
      "Input ROOT file or glob pattern (-i/--input, repeatable)",
      {'i', "input"});
  args::ValueFlag<std::string> output(
      parser, "output", "Output ROOT file (-o/--output)", {'o', "output"});

  try {
    parser.ParseCLI(argc, argv);
  } catch (const args::Help &) {
    std::cout << parser;
    return 0;
  } catch (const args::ParseError &e) {
    std::cerr << e.what() << "\n" << parser;
    return 1;
  } catch (const args::ValidationError &e) {
    std::cerr << e.what() << "\n" << parser;
    return 1;
  }

  if (!inputs) {
    std::cerr << "Error: At least one -i/--input is required\n";
    return 1;
  }
  if (!output) {
    std::cerr << "Error: -o/--output is required\n";
    return 1;
  }

  const auto &patterns = inputs.Get();
  const auto &out_file = output.Get();

  // Expand all patterns / literal paths into a flat file list
  std::vector<std::string> all_files;
  for (const auto &pat : patterns) {
    auto expanded = expand_glob(pat);
    if (expanded.empty()) {
      // Treat as a literal path (no match / no wildcards)
      all_files.push_back(pat);
    } else {
      all_files.insert(all_files.end(), expanded.begin(), expanded.end());
    }
  }

  if (all_files.empty()) {
    std::cerr << "Error: No input files found after expanding patterns\n";
    return 1;
  }

  try {
    // Validate that every file is a proper ROOT file
    for (const auto &f : all_files) {
      TFile tf(f.c_str(), "READ");
      if (tf.IsZombie()) {
        throw std::runtime_error(
            "File is not a valid ROOT file "
            "(SimtelEventSource format is not supported): " +
            f);
      }
      tf.Close();
    }

    spdlog::info("Merging {} file(s) into {}", all_files.size(), out_file);
    SourceMerger merger(out_file);
    merger(all_files, /*is_root=*/true);
    spdlog::info("Merge complete -> {}", out_file);
  } catch (const std::exception &e) {
    spdlog::error("Merge failed: {}", e.what());
    return 2;
  }

  return 0;
}
