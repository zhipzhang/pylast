#include "histogram_helper.hh"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

TEST_CASE("TEST_INTERPOLATE_HISTOGRAM") {
  auto histogram = new TH2D("histogram", "histogram", 4, 0, 4, 4, 0, 4);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i != j)
        histogram->SetBinContent(i + 1, j + 1, j + 1);
      else
        histogram->SetBinContent(i + 1, j + 1, -1);
    }
  }
  CHECK(interpolate_histogram(histogram, 1, 1, -1).inside == false);
  CHECK(interpolate_histogram(histogram, 1.5, 0, -1).bilinear == false);
  CHECK(interpolate_histogram(histogram, 1.5, 0.1, -1).inside == true);
  CHECK(interpolate_histogram(histogram, 1.5, 0, -1).value == 1);

  CHECK(interpolate_histogram(histogram, 3, 1, -1).inside == true);
  CHECK(interpolate_histogram(histogram, 3, 1, -1).bilinear == true);
  CHECK(interpolate_histogram(histogram, 3, 1, -1).value == 1.5);
}