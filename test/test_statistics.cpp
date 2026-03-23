#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Statistics.hh"
#include "doctest/doctest.h"



TEST_CASE("Testing Statistics addition operator") {
    // Create two Statistics objects
    Statistics stats1;
    Statistics stats2;
    
    // Add 1D histograms to both statistics objects
    auto hist1d_1 = make_regular_histogram<float>(0.0f, 10.0f, 10);
    auto hist1d_2 = make_regular_histogram<float>(0.0f, 10.0f, 10);
    
    // Fill histograms with some data
    hist1d_1.fill(2.5f, 1.0f);
    hist1d_1.fill(5.5f, 2.0f);
    hist1d_2.fill(3.5f, 3.0f);
    hist1d_2.fill(7.5f, 4.0f);
    
    stats1.add_histogram("hist1d", hist1d_1);
    stats2.add_histogram("hist1d", hist1d_2);
    
    // Add 2D histograms to both statistics objects
    auto hist2d_1 = make_regular_histogram_2d<float>(0.0f, 10.0f, 5, 0.0f, 10.0f, 5);
    auto hist2d_2 = make_regular_histogram_2d<float>(0.0f, 10.0f, 5, 0.0f, 10.0f, 5);
    
    // Fill 2D histograms with some data
    hist2d_1.fill(2.5f, 3.5f, 1.0f);
    hist2d_1.fill(7.5f, 8.5f, 2.0f);
    hist2d_2.fill(4.5f, 5.5f, 3.0f);
    hist2d_2.fill(1.5f, 2.5f, 4.0f);
    
    stats1.add_histogram("hist2d", hist2d_1);
    stats2.add_histogram("hist2d", hist2d_2);
    
    // Test addition operator
    stats1 += stats2;
    
    // Check that the sum has the correct histograms
    CHECK(stats1.histograms.size() == 2);
    CHECK(stats1.histograms.count("hist1d") == 1);
    CHECK(stats1.histograms.count("hist2d") == 1);
    
    // Check 1D histogram addition
    auto hist1d_sum = std::dynamic_pointer_cast<Histogram1D<float>>(stats1.histograms.at("hist1d"));
    CHECK(hist1d_sum != nullptr);
    CHECK(hist1d_sum->get_bin_content(2) == 1.0f); // bin for 2.5
    CHECK(hist1d_sum->get_bin_content(3) == 3.0f); // bin for 3.5
    CHECK(hist1d_sum->get_bin_content(5) == 2.0f); // bin for 5.5
    CHECK(hist1d_sum->get_bin_content(7) == 4.0f); // bin for 7.5
    
    // Check 2D histogram addition
    auto hist2d_sum = std::dynamic_pointer_cast<Histogram2D<float>>(stats1.histograms.at("hist2d"));
    CHECK(hist2d_sum != nullptr);
    CHECK(hist2d_sum->operator()(0, 1) == 4.0f); // bin for (1.5, 2.5)
    CHECK(hist2d_sum->operator()(1, 1) == 1.0f); // bin for (2.5, 3.5)
    CHECK(hist2d_sum->operator()(2, 2) == 3.0f); // bin for (4.5, 5.5)
    CHECK(hist2d_sum->operator()(3, 4) == 2.0f); // bin for (7.5, 8.5)
    
    // Test adding to an empty Statistics object
    Statistics empty_stats ;
    empty_stats += stats1;
    
    // Check that result has the same histograms as stats1
    CHECK(empty_stats.histograms.size() == stats1.histograms.size());
    CHECK(empty_stats.histograms.count("hist1d") == 1);
    CHECK(empty_stats.histograms.count("hist2d") == 1);
    
    // Check that the histogram contents are the same
    auto hist1d_result = std::dynamic_pointer_cast<Histogram1D<float>>(empty_stats.histograms.at("hist1d"));
    CHECK(hist1d_result != nullptr);
    CHECK(hist1d_result->get_bin_content(2) == 1.0f); // bin for 2.5
    CHECK(hist1d_result->get_bin_content(5) == 2.0f); // bin for 5.5
    
    auto hist2d_result = std::dynamic_pointer_cast<Histogram2D<float>>(empty_stats.histograms.at("hist2d"));
    CHECK(hist2d_result != nullptr);
    CHECK(hist2d_result->operator()(1, 1) == 1.0f); // bin for (2.5, 3.5)
    CHECK(hist2d_result->operator()(3, 4) == 2.0f); // bin for (7.5, 8.5)
}
