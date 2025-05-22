import argparse
from pylast.io import SimtelEventSource
from pylast.reco import ShowerProcessor
from pylast.calib import Calibrator
from pylast.image import ImageProcessor
from pylast.io import DataWriter
from pylast.helper import Statistics, make_regular_histogram, make_regular_histogram2d
import numpy as np
import tqdm
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
    config["image_processor"]["TailcutsCleaner"]["picture_thresh"] = 15.0
    config["image_processor"]["TailcutsCleaner"]["boundary_thresh"] = 7.5
    config["image_processor"]["TailcutsCleaner"]["keep_isolated_pixels"] = False
    config["image_processor"]["TailcutsCleaner"]["min_number_picture_neighbors"] = 2
    
    # Shower reconstruction defaults
    config["shower_processor"]["GeometryReconstructionTypes"] = ["HillasReconstructor"]
    config["shower_processor"]["EnergyReconstructionTypes"] = ["MLEnergyReconstructor"]
    config["shower_processor"]["ParticleClassificationTypes"] = ["MLParticleClassifier"]

    # Image Cuts
    config["shower_processor"]["MLParticleClassifier"] = {}
    config["shower_processor"]["MLParticleClassifier"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3 && hillas_width > 0 && morphology_n_pixels >= 5"
    config["shower_processor"]["HillasReconstructor"] = {}
    config["shower_processor"]["HillasWeightedReconstructor"] = {}
    config["shower_processor"]["HillasReconstructor"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3 && hillas_width > 0 && morphology_n_pixels >= 5"
    config["shower_processor"]["HillasWeightedReconstructor"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3 && hillas_width > 0 && morphology_n_pixels >= 5"
    config["shower_processor"]["MLEnergyReconstructor"] = {}
    config["shower_processor"]["MLEnergyReconstructor"]["ImageQuery"] = "hillas_intensity > 100 && leakage_intensity_width_2 < 0.3 && hillas_width > 0 && morphology_n_pixels >= 5"

    # Convert the dictionary to a JSON-formatted string

    config["data_writer"]["output_type"] = "root"
    config["data_writer"]["eos_url"] = "root://eos01.ihep.ac.cn/"
    config["data_writer"]["overwrite"] = True
    config["data_writer"]["write_simulation_shower"] = True
    config["data_writer"]["write_simulated_camera"] = True
    config["data_writer"]["write_simulated_camera_image"] = False
    config["data_writer"]["write_r0"] = False
    config["data_writer"]["write_r1"] = False
    config["data_writer"]["write_dl0"] = False
    config["data_writer"]["write_dl1"] = True
    config["data_writer"]["write_dl1_image"] = False
    config["data_writer"]["write_dl2"] = True
    config["data_writer"]["write_monitor"] = False
    config["data_writer"]["write_pointing"] = True
    config["data_writer"]["write_simulation_config"] = False
    config["data_writer"]["write_atmosphere_model"] = False
    config["data_writer"]["write_subarray"] = True
    config["data_writer"]["write_metaparam"] = False
    
    return config

def hillas_reco():
    parser = argparse.ArgumentParser(description="Process multiple input files and save results to corresponding output files")
    parser.add_argument("--input", "-i", action="append", required=True, 
                        help="Input file path (can be used multiple times)")
    parser.add_argument("--output", "-o", action="append", required=True,
                        help="Output file path (can be used multiple times)")
    parser.add_argument("--config", "-c", required=False,
                        help="Config file path(If not provided, default config will be used)")
    parser.add_argument("--max-leakage2", "-l", required=False,
                        help="Max leakage2 for hillas reconstruction")
    args = parser.parse_args()
    
    # Verify that input and output lists have the same length
    if len(args.input) != len(args.output):
        raise ValueError("Number of input files must match number of output files")
    if(args.config is None):
        config = default_config()
    else:
        with open(args.config, "r") as f:
            config = json.load(f)
    if(args.max_leakage2 is not None):
        config["shower_processor"]["HillasReconstructor"]["ImageQuery"] = f"leakage_intensity_width_2 < {args.max_leakage2} && hillas_intensity > 100"
    
    # Process each input-output pair
    for input_file, output_file in tqdm.tqdm(zip(args.input, args.output)):
        statistics = Statistics()
        try:
            source = SimtelEventSource(input_file, load_simulated_showers=True)
            simulated_shower_hist = make_regular_histogram(min=-1, max=3, bins=60)
            simulated_shower_hist.fill(np.log10(source.shower_array.energy))
            angular_resolution_versus_energy_hist = make_regular_histogram2d(min_x=-1, max_x=3, bins_x=60, min_y=0, max_y=1, bins_y=1000)
            # Initialize processors
            calibrator = Calibrator(source.subarray, config_str=json.dumps(config["calibrator"]))
            image_processor = ImageProcessor(source.subarray, config_str=json.dumps(config["image_processor"]))
            shower_processor = ShowerProcessor(source.subarray, config_str=json.dumps(config["shower_processor"]))
            data_writer = DataWriter(source, output_file, config_str=json.dumps(config["data_writer"]))
            # Process the file
            for event in source:
                # Write results
                calibrator(event)
                image_processor(event)
                shower_processor(event)
                data_writer(event)
                if(event.dl2.geometry["HillasReconstructor"].is_valid):
                    angular_resolution_versus_energy_hist.fill(np.log10(event.simulation.shower.energy), event.dl2.geometry["HillasReconstructor"].direction_error)
            statistics.add_histogram("Direction Error(deg) versus True Energy(TeV)", angular_resolution_versus_energy_hist)
            statistics.add_histogram("log10(True Energy(TeV))", simulated_shower_hist)
            data_writer.write_statistics(statistics)
        except Exception as e:
            print(f"Error processing {input_file}: {e}")
            # Continue with next file instead of stopping
            continue
            
    print("Processing complete")


if __name__ == "__main__":
    hillas_reco()
