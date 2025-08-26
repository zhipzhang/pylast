#include "RootEventSource.hh"
#include "SimtelEventSource.hh"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "DataWriter.hh"


TEST_CASE("WRITE_ALL_SIMULATION_SHOWER")
{
    auto test_directory = std::filesystem::path(__FILE__);
    auto test_file = test_directory.parent_path() / "test_data" / "lact_prod0_simtel_particle_gamma_energy_1000.0_1000.0_zenith_0.0_azimuth_0.0_run_1_event_0.zst";

    SUBCASE("get_shower_directly")
    {
        SimtelEventSource source(test_file.string(), -1, {}, true);
        DataWriter writer(source, "test1.root");
        writer.write_all_simulation_shower(source.get_shower_array());
        writer.close();
        RootEventSource root_source("test1.root", -1, {}, false);
        CHECK(root_source.get_shower_array().size() == source.get_shower_array().size());
    }
    SUBCASE("get_shower_implicitly")
    {
        SimtelEventSource source(test_file.string(), 10, {}, false);
        for(const auto& event: source) {
        }
        DataWriter writer(source, "test2.root");
        writer.write_all_simulation_shower(source.get_shower_array());
        writer.close();
        RootEventSource root_source("test2.root", -1, {}, false);
        CHECK(root_source.get_shower_array().size() == source.get_shower_array().size());
    }
    

}
