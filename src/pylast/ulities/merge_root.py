import argparse
from pylast.io import RootEventSource
from pylast.io import DataWriter
from pylast.helper import Statistics
import json

def default_config():
    config = {}
    config["data_writer"] = {}
    config["data_writer"]["output_type"] = "root"
    config["data_writer"]["eos_url"] = "root://eos01.ihep.ac.cn/"
    config["data_writer"]["overwrite"] = True
    config["data_writer"]["write_simulation_shower"] = False
    config["data_writer"]["write_simulated_camera"] = False
    config["data_writer"]["write_r0"] = False
    config["data_writer"]["write_r1"] = False
    config["data_writer"]["write_dl0"] = False
    config["data_writer"]["write_dl1"] = True
    config["data_writer"]["write_dl1_image"] = False
    config["data_writer"]["write_dl2"] = True
    config["data_writer"]["write_monitor"] = False
    config["data_writer"]["write_pointing"] = True
    config["data_writer"]["write_simulation_config"] = True
    config["data_writer"]["write_atmosphere_model"] = True
    config["data_writer"]["write_subarray"] = True
    config["data_writer"]["write_metaparam"] = False
    
    return config

def merge_root():
    parser = argparse.ArgumentParser(description="Process multiple input files and save results to corresponding output files")
    parser.add_argument("--input", "-i", action="append", required=True, 
                        help="Input file path (can be used multiple times)")
    parser.add_argument("--output", "-o", required=True,
                        help="Output file path (can be used multiple times)")
    args = parser.parse_args()


    statistics = Statistics()

    for i, input_file in enumerate(args.input):
        source = RootEventSource(input_file)
        if i == 0:
            data_writer = DataWriter(source, config_str=json.dumps(default_config()["data_writer"]), filename=args.output)
        if(source.statistics is not None):
            statistics += source.statistics
        for event in source:
            data_writer(event)
    data_writer.write_statistics(statistics)
    data_writer.close()


