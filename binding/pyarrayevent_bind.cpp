#include "R0Event.hh"
#include "nanobind/nanobind.h"
#include "ArrayEvent.hh"
#include "SimulatedShowerArray.hh"
#include "nanobind/eigen/dense.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/optional.h"
#include "nanobind/stl/array.h"
#include "nanobind/stl/unique_ptr.h"
#include "nanobind/stl/map.h"
#include "nanobind/stl/vector.h"
#include "TelMonitor.hh"
#include "EventMonitor.hh"
#include "DL0Event.hh"
#include "DL1Event.hh"
#include "DL2Event.hh"
#include "spdlog/spdlog.h"
namespace nb = nanobind;

void bind_dl2_event(nb::module_ &m) {
    nb::class_<DL2Event>(m, "DL2Event")
        .def_ro("geometry", &DL2Event::geometry, nb::rv_policy::reference_internal)
        .def_ro("energy", &DL2Event::energy, nb::rv_policy::reference_internal)
        .def_ro("tels", &DL2Event::tels, nb::rv_policy::reference_internal)
        .def("add_geometry", &DL2Event::add_geometry)
        .def("add_energy", &DL2Event::add_energy)
        .def("set_tel_estimate_energy", &DL2Event::set_tel_estimate_energy)
        .def("set_tel_estimate_disp", &DL2Event::set_tel_estimate_disp)
        .def("set_tel_disp", &DL2Event::set_tel_disp)
        .def_rw("hadroness", &DL2Event::hadroness)
        .def("__repr__", [](DL2Event& self) {
            std::string repr = "DL2Event:\n";
            
            // Add geometry information (just names)
            if (!self.geometry.empty()) {
                repr += "  geometry reconstructors: ";
                bool first = true;
                for (const auto& [name, _] : self.geometry) {
                    if (!first) repr += ", ";
                    repr += name;
                    first = false;
                }
                repr += "\n";
            }
            
            // Add telescope IDs (just the IDs)
            if (!self.tels.empty()) {
                repr += "  Telescope IDs: ";
                bool first = true;
                for (const auto& [tel_id, _] : self.tels) {
                    if (!first) repr += ", ";
                    repr += std::to_string(tel_id);
                    first = false;
                }
                repr += "\n";
            }
            
            return repr;
        });
    nb::class_<ReconstructedEnergy>(m, "ReconstructedEnergy")
        .def(nb::init<>())
        .def(nb::init<double, bool>())
        .def_rw("estimate_energy", &ReconstructedEnergy::estimate_energy)
        .def_rw("energy_valid", &ReconstructedEnergy::energy_valid)
        .def("__repr__", [](ReconstructedEnergy& self) {
            return fmt::format("ReconstructedEnergy:\n  estimate_energy: {}\n  energy_valid: {}", self.estimate_energy, self.energy_valid);
        });
    nb::class_<ReconstructedGeometry>(m, "ReconstructedGeometry")
        .def_rw("is_valid", &ReconstructedGeometry::is_valid)
        .def_rw("alt", &ReconstructedGeometry::alt)
        .def_rw("az", &ReconstructedGeometry::az)
        .def_rw("core_x", &ReconstructedGeometry::core_x)
        .def_rw("core_y", &ReconstructedGeometry::core_y)
        .def_rw("core_pos_error", &ReconstructedGeometry::core_pos_error)
        .def_rw("tilted_core_x", &ReconstructedGeometry::tilted_core_x)
        .def_rw("tilted_core_y", &ReconstructedGeometry::tilted_core_y)
        .def_rw("tilted_core_uncertainty_x", &ReconstructedGeometry::tilted_core_uncertainty_x)
        .def_rw("tilted_core_uncertainty_y", &ReconstructedGeometry::tilted_core_uncertainty_y)
        .def_rw("hmax", &ReconstructedGeometry::hmax)
        .def_rw("direction_error", &ReconstructedGeometry::direction_error)
        .def_rw("alt_uncertainty", &ReconstructedGeometry::alt_uncertainty)
        .def_rw("az_uncertainty", &ReconstructedGeometry::az_uncertainty)
        .def_ro("telescopes", &ReconstructedGeometry::telescopes)
        .def("set_telescopes", [](ReconstructedGeometry& self, nb::list telescopes) {
            std::vector<int> tel_vec;
            for (auto item : telescopes) {
                tel_vec.push_back(nb::cast<int>(item));
            }
            self.telescopes = tel_vec;
        })
        .def("__repr__", [](ReconstructedGeometry& self) {
            return fmt::format("ReconstructedGeometry:\n"
                              "  alt: {}\n"
                              "  az: {}\n"
                              "  core_x: {}\n"
                              "  core_y: {}\n"
                              "  core_pos_error: {}\n"
                              "  tilted_core_x: {}\n"
                              "  tilted_core_y: {}\n",
                              self.alt, self.az, self.core_x, self.core_y,
                              self.core_pos_error, self.tilted_core_x, self.tilted_core_y);
        });
    nb::class_<TelReconstructedParameter>(m, "TelReconstructedParameter")
        .def_ro("impact_parameters", &TelReconstructedParameter::impact_parameters)
        .def_ro("disp", &TelReconstructedParameter::disp)
        .def_ro("estimate_energy", &TelReconstructedParameter::estimate_energy)
        .def_prop_ro("impact", [](TelReconstructedParameter& self) -> nb::object {
            if(self.impact_parameters.size() == 1) {
                return nb::cast(self.impact_parameters.begin()->second);
            }
            else {
                return nb::cast(self.impact_parameters);
            }
        })
        .def("__repr__", [](TelReconstructedParameter& self) {
            std::string repr = "TelReconstructedParameter:\n";
            if (!self.impact_parameters.empty()) {
                repr += "  Impact parameters for reconstructors: ";
                bool first = true;
                for (const auto& [name, _] : self.impact_parameters) {
                    if (!first) repr += ", ";
                    repr += name;
                    first = false;
                }
            }
            return repr;
        });
    nb::class_<TelImpactParameter>(m, "TelImpactParameter")
        .def_ro("distance", &TelImpactParameter::distance)
        .def_ro("distance_error", &TelImpactParameter::distance_error)
        .def("__repr__", [](TelImpactParameter& self) {
            return fmt::format("TelImpactParameter:\n  distance: {}\n  distance_error: {}", 
                              self.distance, self.distance_error);
        });
}

void bind_dl1_event(nb::module_ &m) {
    nb::class_<DL1Event>(m, "DL1Event")
        .def_prop_ro("tels", &DL1Event::get_tels)
        .def("__repr__", [](DL1Event& self) {
            std::string repr = "DL1Event:\n";
            auto tels = self.get_tels();
            if (!tels.empty()) {
                repr += "  Telescope IDs: ";
                bool first = true;
                for (const auto& [tel_id, _] : tels) {
                    if (!first) repr += ", ";
                    repr += std::to_string(tel_id);
                    first = false;
                }
                repr += "\n";
            }
            return repr;
        });
    nb::class_<DL1Camera>(m, "DL1Camera")
        .def_ro("image", &DL1Camera::image)
        .def_ro("peak_time", &DL1Camera::peak_time)
        .def_ro("mask", &DL1Camera::mask)
        .def_ro("image_parameters", &DL1Camera::image_parameters)
        .def("__repr__", [](DL1Camera& self) {
            return fmt::format("DL1Camera:\n  image shape: {}x{}\n  peak_time shape: {}x{}\n  mask shape: {}x{}", 
                              self.image.rows(), self.image.cols(),
                              self.peak_time.rows(), self.peak_time.cols(),
                              self.mask.rows(), self.mask.cols());
        });
    nb::class_<ImageParameters>(m, "image_parameters")
        .def_ro("hillas", &ImageParameters::hillas)
        .def_ro("leakage", &ImageParameters::leakage)
        .def_ro("concentration", &ImageParameters::concentration)
        .def_ro("morphology", &ImageParameters::morphology)
        .def_ro("extra", &ImageParameters::extra)
        .def_ro("intensity", &ImageParameters::intensity)
        .def("__repr__", [](ImageParameters& self) {
            return "ImageParameters: contains hillas, leakage, concentration, and morphology parameters";
        });
    nb::class_<MorphologyParameter>(m, "morphology")
        .def_ro("n_pixels", &MorphologyParameter::n_pixels)
        .def_ro("n_islands", &MorphologyParameter::n_islands)
        .def_ro("n_small_islands", &MorphologyParameter::n_small_islands)
        .def_ro("n_medium_islands", &MorphologyParameter::n_medium_islands)
        .def_ro("n_large_islands", &MorphologyParameter::n_large_islands)
        .def("__repr__", [](MorphologyParameter& self) {
            return fmt::format("MorphologyParameter:\n  n_pixels: {}\n  n_islands: {}\n  n_small_islands: {}\n  n_medium_islands: {}\n  n_large_islands: {}", 
                              self.n_pixels, self.n_islands, self.n_small_islands, 
                              self.n_medium_islands, self.n_large_islands);
        });
    nb::class_<HillasParameter>(m, "hillas")
        .def_ro("x", &HillasParameter::x)
        .def_ro("y", &HillasParameter::y)
        .def_ro("width", &HillasParameter::width)
        .def_ro("length", &HillasParameter::length)
        .def_ro("phi", &HillasParameter::phi)
        .def_ro("psi", &HillasParameter::psi)
        .def_ro("intensity", &HillasParameter::intensity)
        .def_ro("skewness", &HillasParameter::skewness)
        .def_ro("kurtosis", &HillasParameter::kurtosis)
        .def_ro("r", &HillasParameter::r)
        .def("__repr__", [](HillasParameter& self) {
            return fmt::format("HillasParameter:\n  x: {}\n  y: {}\n  width: {}\n  length: {}\n  phi: {}\n  intensity: {}", 
                              self.x, self.y, self.width, self.length, self.phi, self.intensity);
        });
    nb::class_<LeakageParameter>(m, "leakage")
        .def_ro("pixels_width_1", &LeakageParameter::pixels_width_1)
        .def_ro("pixels_width_2", &LeakageParameter::pixels_width_2)
        .def_ro("intensity_width_1", &LeakageParameter::intensity_width_1)
        .def_ro("intensity_width_2", &LeakageParameter::intensity_width_2)
        .def("__repr__", [](LeakageParameter& self) {
            return fmt::format("LeakageParameter:\n  pixels_width_1: {}\n  pixels_width_2: {}\n  intensity_width_1: {}\n  intensity_width_2: {}", 
                              self.pixels_width_1, self.pixels_width_2, 
                              self.intensity_width_1, self.intensity_width_2);
        });
    nb::class_<ConcentrationParameter>(m, "concentration")
        .def_ro("concentration_cog", &ConcentrationParameter::concentration_cog)
        .def_ro("concentration_core", &ConcentrationParameter::concentration_core)
        .def_ro("concentration_pixel", &ConcentrationParameter::concentration_pixel)
        .def("__repr__", [](ConcentrationParameter& self) {
            return fmt::format("ConcentrationParameter:\n  concentration_cog: {}\n  concentration_core: {}\n  concentration_pixel: {}", 
                              self.concentration_cog, self.concentration_core, self.concentration_pixel);
        });
    nb::class_<ExtraParameters>(m, "extra")
        .def_ro("miss", &ExtraParameters::miss)
        .def_ro("disp", &ExtraParameters::disp)
        .def_ro("theta", &ExtraParameters::theta)
        .def("__repr__", [](ExtraParameters& self) {
            return fmt::format("ExtraParameters:\n  miss: {}\n  disp: {}\n  theta: {}", self.miss, self.disp, self.theta);
        });
    nb::class_<IntensityParameter>(m, "intensity")
        .def_ro("intensity_max", &IntensityParameter::intensity_max)
        .def_ro("intensity_mean", &IntensityParameter::intensity_mean)
        .def_ro("intensity_std", &IntensityParameter::intensity_std)
        .def("__repr__", [](IntensityParameter& self) {
            return fmt::format("IntensityParameter:\n  intensity_max: {}\n  intensity_mean: {}\n  intensity_std: {}", self.intensity_max, self.intensity_mean, self.intensity_std);
        });
}
void bind_dl0_event(nb::module_ &m) {
    nb::class_<DL0Event>(m, "DL0Event")
        .def_prop_ro("tels", &DL0Event::get_tels)
        .def("__repr__", [](DL0Event& self) {
            std::string repr = "DL0Event:\n";
            auto tels = self.get_tels();
            if (!tels.empty()) {
                repr += "  Telescope IDs: ";
                bool first = true;
                for (const auto& [tel_id, _] : tels) {
                    if (!first) repr += ", ";
                    repr += std::to_string(tel_id);
                    first = false;
                }
                repr += "\n";
            }
            return repr;
        });
    nb::class_<DL0Camera>(m, "DL0Camera")
        .def_ro("image", &DL0Camera::image)
        .def_ro("peak_time", &DL0Camera::peak_time)
        .def("__repr__", [](DL0Camera& self) {
            return fmt::format("DL0Camera:\n  image shape: {}x{}\n  peak_time shape: {}x{}", 
                              self.image.rows(), self.image.cols(),
                              self.peak_time.rows(), self.peak_time.cols());
        });
}
void bind_r1_event(nb::module_ &m) {
    nb::class_<R1Event>(m, "R1Event")
        .def_prop_ro("tels", &R1Event::get_tels)
        .def("__repr__", [](R1Event& self) {
            std::string repr = "R1Event:\n";
            auto tels = self.get_tels();
            if (!tels.empty()) {
                repr += "  Telescope IDs: ";
                bool first = true;
                for (const auto& [tel_id, _] : tels) {
                    if (!first) repr += ", ";
                    repr += std::to_string(tel_id);
                    first = false;
                }
                repr += "\n";
            }
            return repr;
        });
    nb::class_<R1Camera>(m, "R1Camera")
        .def_ro("waveform", &R1Camera::waveform)
        .def_ro("gain_selection", &R1Camera::gain_selection)
        .def("__repr__", [](R1Camera& self) {
            return fmt::format("R1Camera:\n  waveform shape: {}x{}\n  gain_selection shape: {}x{}", 
                              self.waveform.rows(), self.waveform.cols(),
                              self.gain_selection.rows(), self.gain_selection.cols());
        });
}
void bind_r0_event(nb::module_ &m) {
    nb::class_<R0Event>(m, "R0Event")
        .def_prop_ro("tels", &R0Event::get_tels)
        .def("__repr__", [](R0Event& self) {
            std::string repr = "R0Event:\n";
            auto tels = self.get_tels();
            if (!tels.empty()) {
                repr += "  Telescope IDs: ";
                bool first = true;
                for (const auto& [tel_id, _] : tels) {
                    if (!first) repr += ", ";
                    repr += std::to_string(tel_id);
                    first = false;
                }
                repr += "\n";
            }
            return repr;
        });
    nb::class_<R0Camera>(m, "R0Camera")
        .def_ro("waveform", &R0Camera::waveform)
        .def_ro("waveform_sum", &R0Camera::waveform_sum)
        .def("__repr__", [](R0Camera& self) {
            return fmt::format("R0Camera:\n  waveform shape: 2x{}x{}\n  waveform_sum shape:2x{}x{}", 
                              self.waveform[0].rows(), self.waveform[0].cols(),
                              (*self.waveform_sum)[0].rows(), (*self.waveform_sum)[0].cols());
        });
}
void bind_simulated_event(nb::module_ &m) {
    nb::class_<SimulatedEvent>(m, "SimulatedEvent")
        .def_ro("shower", &SimulatedEvent::shower)
        .def_prop_ro("tels", &SimulatedEvent::get_tels)
        .def("__repr__", [](SimulatedEvent& self) {
            std::string repr = "SimulatedEvent:\n";
            auto tels = self.get_tels();
            if (!tels.empty()) {
                repr += "  Telescope IDs: ";
                bool first = true;
                for (const auto& [tel_id, _] : tels) {
                    if (!first) repr += ", ";
                    repr += std::to_string(tel_id);
                    first = false;
                }
                repr += "\n";
            }
            return repr;
        });
    nb::class_<SimulatedCamera>(m, "SimulatedCamera")
        .def_ro("true_image_sum", &SimulatedCamera::true_image_sum)
        .def_ro("true_image", &SimulatedCamera::true_image)
        .def_ro("impact", &SimulatedCamera::impact)
        .def("__repr__", &SimulatedCamera::print);
   // nb::class_<TelImpactParameter>(m, "TelImpactParameter")
   //     .def_ro("impact_distance", &TelImpactParameter::distance)
   //     .def_ro("impact_distance_error", &TelImpactParameter::distance_error);
    nb::class_<SimulatedShower>(m, "SimulatedShower")
        .def_ro("alt", &SimulatedShower::alt)
        .def_ro("az", &SimulatedShower::az)
        .def_ro("core_x", &SimulatedShower::core_x)
        .def_ro("core_y", &SimulatedShower::core_y)
        .def_ro("energy", &SimulatedShower::energy)
        .def_ro("h_first_int", &SimulatedShower::h_first_int)
        .def_ro("x_max", &SimulatedShower::x_max)
        .def_ro("starting_grammage", &SimulatedShower::starting_grammage)
        .def_ro("shower_primary_id", &SimulatedShower::shower_primary_id)
        .def("__repr__", &SimulatedShower::print);
}
void bind_tel_monitor(nb::module_ &m) {
    nb::class_<EventMonitor>(m, "EventMonitor")
        .def_prop_ro("tels", &EventMonitor::get_tels)
        .def("__repr__", [](EventMonitor& self) {
            std::string repr = "EventMonitor:\n";
            auto tels = self.get_tels();
            if (!tels.empty()) {
                repr += "  Telescope IDs: ";
                bool first = true;
                for (const auto& [tel_id, _] : tels) {
                    if (!first) repr += ", ";
                    repr += std::to_string(tel_id);
                    first = false;
                }
                repr += "\n";
            }
            return repr;
        });
    nb::class_<TelMonitor>(m, "TelMonitor")
        .def_ro("n_channels", &TelMonitor::n_channels)
        .def_ro("n_pixels", &TelMonitor::n_pixels)
        .def_ro("pedestal_per_sample", &TelMonitor::pedestal_per_sample)
        .def_ro("dc_to_pe", &TelMonitor::dc_to_pe)
        .def("__repr__", [](TelMonitor& self) {
            return fmt::format("TelMonitor:\n  n_channels: {}\n  n_pixels: {}", 
                              self.n_channels, self.n_pixels);
        });
}
        
void bind_pointing_event(nb::module_ &m) {
    nb::class_<Pointing>(m, "Pointing")
        .def_ro("array_azimuth", &Pointing::array_azimuth)
        .def_ro("array_altitude", &Pointing::array_altitude)
        .def("__repr__", [](Pointing& self) {
            return fmt::format("Pointing:\n  array_azimuth: {}\n  array_altitude: {}", self.array_azimuth, self.array_altitude);
        });
}
void bind_array_event(nb::module_ &m) {
    nb::class_<ArrayEvent>(m, "ArrayEvent")
        .def_ro("simulation", &ArrayEvent::simulation)
        .def_ro("r0", &ArrayEvent::r0)
        .def_ro("monitor", &ArrayEvent::monitor)
        .def_ro("r1", &ArrayEvent::r1)
        .def_ro("dl0", &ArrayEvent::dl0)
        .def_ro("dl1", &ArrayEvent::dl1)
        .def_ro("dl2", &ArrayEvent::dl2, nb::rv_policy::reference_internal)
        .def_ro("pointing", &ArrayEvent::pointing)
        .def_ro("event_id", &ArrayEvent::event_id)
        .def_ro("run_id", &ArrayEvent::run_id)
        .def("__repr__", [](ArrayEvent& self) {
            std::string repr = "ArrayEvent:\n";
            
            if(self.simulation.has_value()) {
                repr += "  Simulation: Available\n";
            }
            
            if(self.r0.has_value()) {
                repr += "  R0 Data: Available\n";
            }
            
            if(self.r1.has_value()) {
                repr += "  R1 Data: Available\n";
            }
            
            if(self.dl0.has_value()) {
                repr += "  DL0 Data: Available\n";
            }
            
            if(self.dl1.has_value()) {
                repr += "  DL1 Data: Available\n";
            }
            
            if(self.dl2.has_value()) {
                repr += "  DL2 Data: Available\n";
            }
            
            if(self.monitor.has_value()) {
                repr += "  Monitor Data: Available\n";
            }
            
            return repr;
        });
}
void bind_simulated_shower_array(nb::module_ &m) {
    nb::class_<SimulatedShowerArray>(m, "SimulatedShowerArray")
        .def_prop_ro("size", &SimulatedShowerArray::size)
        .def_prop_ro("energy", &SimulatedShowerArray::energy)
        .def_prop_ro("alt", &SimulatedShowerArray::alt)
        .def_prop_ro("az", &SimulatedShowerArray::az)
        .def_prop_ro("core_x", &SimulatedShowerArray::core_x)
        .def_prop_ro("core_y", &SimulatedShowerArray::core_y)
        .def_prop_ro("h_first_int", &SimulatedShowerArray::h_first_int)
        .def_prop_ro("x_max", &SimulatedShowerArray::x_max)
        .def_prop_ro("starting_grammage", &SimulatedShowerArray::starting_grammage)
        .def_prop_ro("shower_primary_id", &SimulatedShowerArray::shower_primary_id)
        .def("__repr__", &SimulatedShowerArray::print)
        .def("__getitem__", &SimulatedShowerArray::operator[])
        .def("__len__", &SimulatedShowerArray::size);
}
NB_MODULE(_pylast_arrayevent, m) {
    bind_dl0_event(m);
    bind_dl1_event(m);
    bind_r0_event(m);
    bind_r1_event(m);
    bind_pointing_event(m);
    bind_dl2_event(m);
    bind_simulated_event(m);
    bind_tel_monitor(m);
    bind_array_event(m);
    bind_simulated_shower_array(m);
    
}