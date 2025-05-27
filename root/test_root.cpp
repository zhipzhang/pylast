#include "ImageExtractor.hh"
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include "SimtelEventSource.hh"
#include <iostream>
#include "DataWriter.hh"
#include "Calibration.hh"
#include "ImageProcessor.hh"
#include "ShowerProcessor.hh"
#include "RootWriter.hh"
#include <memory>
int main(int argc, char** argv) {
    auto simtel_source = new SimtelEventSource(argv[1], 10);
    std::unique_ptr<Calibrator> calibration = std::make_unique<Calibrator>(simtel_source->subarray.value());
    std::unique_ptr<ImageProcessor> image_processor = std::make_unique<ImageProcessor>(simtel_source->subarray.value());
    auto shower_processor = std::make_unique<ShowerProcessor>(simtel_source->subarray.value());
    for(auto& event : *simtel_source)
    {
        (*calibration)(event);
        (*image_processor)(event);
        (*shower_processor)(event);
    }
    delete simtel_source;
}