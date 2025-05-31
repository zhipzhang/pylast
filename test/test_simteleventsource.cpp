#include <stdexcept>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "SimtelEventSource.hh"
#include <filesystem>

TEST_CASE("SimtelEventSourceInitialize")
{
    SUBCASE("FileNotExist")
    {
        CHECK_THROWS_AS(SimtelEventSource("not_exist.simtel.zst"), std::runtime_error);
    }
    SUBCASE("test_initialize")
    {
        auto test_directory = std::filesystem::path(__FILE__);
        auto test_file = test_directory.parent_path() / "test_data" / "simtel.zst";
        SimtelEventSource simtel_event_source(test_file.string());
        CHECK(simtel_event_source.is_stream == true);
        CHECK(simtel_event_source.metaparam.has_value() == true);
        CHECK(simtel_event_source.atmosphere_model.has_value() == true);
        CHECK(simtel_event_source.simulation_config.has_value() == true);

        // LACT Telescope in the test file have 32 telescopes.
        CHECK(simtel_event_source.subarray->tels.size() == 32);
        CHECK(simtel_event_source.subarray->tel_positions.size()  == 32);
    }
    SUBCASE("test_max_events")
    {
        auto test_directory = std::filesystem::path(__FILE__);
        auto test_file = test_directory.parent_path() / "test_data" / "simtel.zst";
        SimtelEventSource simtel_event_source(test_file.string(), 10);
        CHECK(simtel_event_source.max_events == 10);
        CHECK(simtel_event_source.is_stream == true);
        
        int all_event = 0;
        for(const auto& event: simtel_event_source) {
            all_event++;
        }
        CHECK(all_event == 10);
        SUBCASE("test_random_access")
        {
            CHECK_THROWS_AS(simtel_event_source[4], std::runtime_error);
            CHECK_THROWS_AS(simtel_event_source[11], std::out_of_range);
        }
    }
    SUBCASE("test_subarray")
    {
        auto test_directory = std::filesystem::path(__FILE__);
        auto test_file = test_directory.parent_path() / "test_data" / "simtel.zst";
        SimtelEventSource simtel_event_source(test_file.string(), -1, {1, 2, 3});
        CHECK(simtel_event_source.allowed_tels.size() == 3);
        CHECK(simtel_event_source.subarray->tels.size() == 3);
        CHECK(simtel_event_source.subarray->tel_positions.size() == 3);
    }
    
}