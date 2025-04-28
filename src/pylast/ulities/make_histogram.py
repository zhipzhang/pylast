import argparse
from pylast.io import RootEventSource, SimtelEventSource
from pylast.histogram.base_histogram import BaseHistogramProfile
from pylast.helper import Statistics, compute_angle_separation, write_statistics
from pylast.calib import Calibrator
from pylast.image import ImageProcessor
from pylast.reco import ShowerProcessor
from pylast.io import DataWriter
import numpy as np
import json

def default_config():
    """
    Returns a default configuration as a JSON-formatted string for the Hillas reconstruction.
    This can be used when no config file is provided.
    """
    config = {}
    
    # Create empty dictionaries for various configuration parameters
    config["calibrator"] = {}
    config["image_processor"] = {}
    config["shower_processor"] = {}
    config["data_writer"] = {}
    
    # Calibration defaults
    config["calibrator"]["image_extractor_type"] = "LocalPeakExtractor"
    config["calibrator"]["LocalPeakExtractor"] = {}
    config["calibrator"]["LocalPeakExtractor"]["window_shift"] = 3
    config["calibrator"]["LocalPeakExtractor"]["window_width"] = 7
    config["calibrator"]["LocalPeakExtractor"]["apply_correction"] = True
    
    # Image cleaning defaults
    config["image_processor"]["image_cleaner_type"] = "Tailcuts_cleaner"
    config["image_processor"]["TailcutsCleaner"] = {}
    config["image_processor"]["TailcutsCleaner"]["picture_thresh"] = 10.0
    config["image_processor"]["TailcutsCleaner"]["boundary_thresh"] = 5.0
    config["image_processor"]["TailcutsCleaner"]["keep_isolated_pixels"] = False
    config["image_processor"]["TailcutsCleaner"]["min_number_picture_neighbors"] = 2
    
    # Shower reconstruction defaults
    config["shower_processor"]["GeometryReconstructionTypes"] = ["HillasReconstructor", "DispStereoReconstructor"]
    config["shower_processor"]["EnergyReconstructionTypes"] = ["MLEnergyReconstructor"]
    config["shower_processor"]["HillasReconstructor"] = {}
    config["shower_processor"]["HillasReconstructor"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3"
    config["shower_processor"]["MLEnergyReconstructor"] = {}
    config["shower_processor"]["MLEnergyReconstructor"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3"
    config["shower_processor"]["DispStereoReconstructor"] = {}
    config["shower_processor"]["DispStereoReconstructor"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3"
    # Convert the dictionary to a JSON-formatted string

    config["data_writer"]["output_type"] = "root"
    config["data_writer"]["eos_url"] = "root://eos01.ihep.ac.cn/"
    config["data_writer"]["overwrite"] = True
    config["data_writer"]["write_simulation_shower"] = True
    config["data_writer"]["write_simulated_camera"] = False
    config["data_writer"]["write_r0"] = False
    config["data_writer"]["write_r1"] = False
    config["data_writer"]["write_dl0"] = False
    config["data_writer"]["write_dl1"] = True
    config["data_writer"]["write_dl1_image"] = False
    config["data_writer"]["write_dl2"] = True
    config["data_writer"]["write_monitor"] = False
    config["data_writer"]["write_pointing"] = False
    config["data_writer"]["write_simulation_config"] = True
    config["data_writer"]["write_atmosphere_model"] = True
    config["data_writer"]["write_subarray"] = True
    config["data_writer"]["write_metaparam"] = False
    
    return config
# Initialize the histogram you needed
def initialize_histogram():
    energy_range = np.logspace(-1, 3, 20) 
    impact_parameter_range = np.linspace(0, 600, 30)
    offset_angle_range = np.linspace(0, 5, 5)
    intensity_histogram = BaseHistogramProfile("intensity", energy_range, offset_angle_range)
    disp_histogram = BaseHistogramProfile("disp", energy_range, offset_angle_range)
    theta_histogram = BaseHistogramProfile("theta", energy_range, offset_angle_range)
    miss_histogram = BaseHistogramProfile("miss", energy_range, offset_angle_range)
    length_histogram = BaseHistogramProfile("length", energy_range, offset_angle_range)
    width_histogram = BaseHistogramProfile("width", energy_range, offset_angle_range)
    return {
        "intensity": intensity_histogram,
        "disp": disp_histogram,
        "theta": theta_histogram,
        "miss": miss_histogram,
        "length": length_histogram,
        "width": width_histogram
    }


def make_histogram():
    histograms = initialize_histogram()
    statistics = Statistics()
    parser = argparse.ArgumentParser("Generate histograms for pylast")
    parser.add_argument("--input", "-i", action="append", required=True, 
                        help="Input file path (can be used multiple times)")
    parser.add_argument("--output", "-o",required=True,
                        help="Output file path (can be used multiple times)")
    args = parser.parse_args()
    histograms = initialize_histogram()
    for input_file in args.input:
        try:
            source = RootEventSource(input_file)
        except Exception as e:
            source = SimtelEventSource(input_file)
            config = default_config()
            calibrator = Calibrator(source.subarray,config_str=json.dumps(config["calibrator"]))
            image_processor = ImageProcessor(source.subarray,config_str=json.dumps(config["image_processor"]))
            shower_processor = ShowerProcessor(source.subarray,config_str=json.dumps(config["shower_processor"]))
        
        if (type(source) == SimtelEventSource):
            for event in source:
                calibrator(event)
                image_processor(event)
                shower_processor(event)
                offset_angle = compute_angle_separation(event.pointing.array_altitude, event.pointing.array_azimuth, event.simulation.shower.alt, event.simulation.shower.az)
                for tel_id, dl1 in event.dl1.tels.items():
                    histograms["intensity"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.hillas.intensity)
                    histograms["disp"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.extra.disp)
                    histograms["theta"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.extra.theta)
                    histograms["miss"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.extra.miss)
                    histograms["length"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.hillas.length)
                    histograms["width"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.hillas.width)
        elif (type(source) == RootEventSource):
            for event in source:
                offset_angle = compute_angle_separation(event.pointing.array_altitude, event.pointing.array_azimuth, event.simulation.shower.altitude, event.simulation.shower.azimuth)
                for tel_id, dl1 in event.dl1.tels.items():
                    histograms["intensity"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.hillas.intensity)
                    histograms["disp"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.extra.disp)
                    histograms["theta"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.extra.theta)
                    histograms["miss"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.extra.miss)
                    histograms["length"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.hillas.length)
                    histograms["width"].fill(event.simulation.shower.energy, offset_angle, event.simulation.tels[tel_id].impact.distance, dl1.image_parameters.hillas.width)
    for key, histogram in histograms.items():
        for name, hist in histogram.get_all_histograms().items():
            statistics.add_histogram(name, hist)
    write_statistics(statistics, args.output)


    
    


                

if __name__ == "__main__":
    make_histogram()