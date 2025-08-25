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
    auto test_directory = std::filesystem::path(__FILE__);
    auto test_file = test_directory.parent_path() / "test_data" / "lact_prod0_simtel_particle_gamma_energy_1000.0_1000.0_zenith_0.0_azimuth_0.0_run_1_event_0.zst";
    SUBCASE("test_initialize")
    {
        SimtelEventSource simtel_event_source(test_file.string());
        CHECK(simtel_event_source.is_stream == true);
        CHECK(simtel_event_source.metaparam.has_value() == true);
        CHECK(simtel_event_source.atmosphere_model.has_value() == true);
        CHECK(simtel_event_source.simulation_config.has_value() == true);

        //  test file have 16 telescopes.
        CHECK(simtel_event_source.subarray->tels.size() == 16);
        CHECK(simtel_event_source.subarray->tel_positions.size()  == 16);
    }
    SUBCASE("test_max_events")
    {
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
        SimtelEventSource simtel_event_source(test_file.string(), -1, {1, 2, 3});
        CHECK(simtel_event_source.allowed_tels.size() == 3);
        CHECK(simtel_event_source.subarray->tels.size() == 3);
        CHECK(simtel_event_source.subarray->tel_positions.size() == 3);
    }
    SUBCASE("test_load_simulated_shower_true")
    {
        SimtelEventSource simtel_event_source(test_file.string(), -1, {}, true);
        CHECK(simtel_event_source.shower_array.has_value() == true);
        CHECK(simtel_event_source.shower_array->size() > 0);
    }
    SUBCASE("test_load_simulated_shower_false")
    {
        SimtelEventSource simtel_event_source(test_file.string(), 10, {}, false);
        CHECK(simtel_event_source.shower_array.has_value() == false);
        for(const auto& event: simtel_event_source) {
            // do no things
        }
        CHECK(simtel_event_source.get_shower_array().size() > 0);
    }
    
}