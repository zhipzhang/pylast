#include "RootEventSource.hh"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "TFileMerger.h"

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::filesystem::path make_merged_root_file(
    const std::vector<std::filesystem::path>& input_files,
    const std::string& tag)
{
    if(input_files.empty())
    {
        throw std::runtime_error("input_files must not be empty");
    }

    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto output_file = std::filesystem::temp_directory_path() /
                             ("pylast_build_index_" + tag + "_" + std::to_string(now) + ".root");

    TFileMerger merger;
    if(!merger.OutputFile(output_file.string().c_str(), true))
    {
        throw std::runtime_error("failed to create merged ROOT file: " + output_file.string());
    }
    for(const auto& input_file : input_files)
    {
        if(!merger.AddFile(input_file.string().c_str()))
        {
            throw std::runtime_error("failed to add input ROOT file: " + input_file.string());
        }
    }
    if(!merger.Merge())
    {
        throw std::runtime_error("failed to merge ROOT files");
    }
    return output_file;
}
} // namespace

TEST_CASE("ROOT_EVENT_SOURCE_RECOVERS_MISSING_INDEX_AND_DETECTS_DUPLICATE_KEYS")
{
    const auto test_directory = std::filesystem::path(__FILE__).parent_path();
    const auto test_data_dir = test_directory / "test_data";
    const auto source_1 = test_data_dir / "root_source_1.root";
    const auto source_2 = test_data_dir / "root_source_2.root";

    SUBCASE("duplicate keys after hadd should throw")
    {
        const auto merged_duplicate = make_merged_root_file({source_1, source_1}, "duplicate");
        CHECK_THROWS_AS(
            [&merged_duplicate]() {
                RootEventSource source(merged_duplicate.string(), -1, {}, false);
                (void)source;
            }(),
            std::runtime_error
        );
        std::filesystem::remove(merged_duplicate);
    }

    SUBCASE("different files after hadd should be readable")
    {
        const auto merged_non_duplicate = make_merged_root_file({source_1, source_2}, "non_duplicate");
        CHECK_NOTHROW(
            [&merged_non_duplicate]() {
                RootEventSource source(merged_non_duplicate.string(), -1, {}, false);
                const auto event = source.get_event(0);
                CHECK(event.event_id >= 0);
            }()
        );
        std::filesystem::remove(merged_non_duplicate);
    }
}
