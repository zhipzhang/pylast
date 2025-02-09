#include "Calibration.hh"
#include "ImageExtractor.hh"
#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/pair.h"
#include "SubarrayDescription.hh"
#include "nanobind/stl/unique_ptr.h"
#include <iostream>
namespace nb = nanobind;

void bind_calibrator(nb::module_ &m)
{
    nb::class_<Calibrator>(m, "Calibrator")
        .def("__init__", [](Calibrator &self, SubarrayDescription& subarray, nb::args args) {
            printf("Constructing Calibrator\n");
            fflush(stdout);
            std::string image_extractor_type = "FullWaveFormExtractor";
            if(args.size() > 0)
            {
                nb::object arg = args[0];
                image_extractor_type = std::string(nb::str(arg).c_str());
            }
            printf("image_extractor_type: %s\n", image_extractor_type.c_str());
            if(image_extractor_type == "FullWaveFormExtractor")
            {
                new (&self) Calibrator(subarray, image_extractor_type);
            }
            else if(image_extractor_type == "LocalPeakExtractor")
            {
                if(args.size() < 3)
                {
                    throw std::runtime_error("LocalPeakExtractor requires at least 3 arguments");
                }
                int window_width = static_cast<int>(nb::int_(args[1]));
                int window_shift = static_cast<int>(nb::int_(args[2]));
                bool use_correction = true;
                if(args.size() == 4)
                {
                     use_correction = static_cast<bool>(nb::bool_(args[3]));
                }
                new (&self) Calibrator(subarray, image_extractor_type, window_width, window_shift, use_correction);
            }
            else
            {
                throw std::runtime_error("Invalid image extractor type");
            }
        })
        .def("__call__", [](Calibrator& self, ArrayEvent& event) {
            self(event);
        });
}