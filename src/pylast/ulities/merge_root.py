import argparse
from pylast.io import RootEventSource
from pylast.io import DataWriter
from pylast.helper import Statistics
import json
import multiprocessing
from functools import partial
import os
import tqdm

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
    config["data_writer"]["write_dl1_image"] = True
    config["data_writer"]["write_dl2"] = True
    config["data_writer"]["write_monitor"] = False
    config["data_writer"]["write_pointing"] = True
    config["data_writer"]["write_simulation_config"] = True
    config["data_writer"]["write_atmosphere_model"] = True
    config["data_writer"]["write_subarray"] = True
    config["data_writer"]["write_metaparam"] = False
    
    return config

def process_file_batch(input_files_batch, temp_dir, config_str):
    """Process a batch of input files and return statistics"""
    try:
        # Create a unique temporary output file for this process
        temp_output = os.path.join(temp_dir, f"temp_{os.getpid()}_batch.root")
        
        # Process the first file to initialize the writer
        first_source = RootEventSource(input_files_batch[0])
        data_writer = DataWriter(first_source, config_str=config_str, filename=temp_output)
        
        # Process all files in the batch
        for i, input_file in enumerate(input_files_batch):
            try:
                source = RootEventSource(input_file)
                
                # Write statistics for the first file
                if i == 0 and source.statistics is not None:
                    data_writer.write_statistics(source.statistics)
                
                # Process all events in the file
                for event in source:
                    data_writer(event)
                    
            except Exception as e:
                print(f"Error processing file in batch {input_file}: {e}")
                continue
            
        data_writer.close()
        return temp_output
    except Exception as e:
        print(f"Error processing batch: {e}")
        return None

def merge_root():
    parser = argparse.ArgumentParser(description="Process multiple input files and save results to corresponding output files")
    parser.add_argument("--input", "-i", action="append", 
                        help="Input file path (can be used multiple times)")
    parser.add_argument("--input-list", "-l", 
                        help="Text file containing a list of input files (one per line)")
    parser.add_argument("--output", "-o", required=True,
                        help="Output file path")
    parser.add_argument("--jobs", "-j", type=int, default=1,
                        help="Number of parallel processes to use")
    args = parser.parse_args()

    input_files = []
    
    # Get input files from command line arguments
    if args.input:
        input_files.extend(args.input)
    
    # Get input files from text file if provided
    if args.input_list:
        with open(args.input_list, 'r') as file_list:
            for line in file_list:
                # Strip whitespace and skip empty lines
                file_path = line.strip()
                if file_path:
                    input_files.append(file_path)
    
    if not input_files:
        parser.error("No input files provided. Use --input or --input-list")

    # If using multiple processes
    if args.jobs > 1:
        # Create a temporary directory for intermediate files
        temp_dir = os.path.join(os.path.dirname(args.output), f"temp_merge_{os.getpid()}")
        os.makedirs(temp_dir, exist_ok=True)
        
        try:
            # Divide input files into batches for each process
            num_processes = min(args.jobs, len(input_files))
            batch_size = len(input_files) // num_processes
            if batch_size == 0:
                batch_size = 1
            
            # Create batches of files
            file_batches = []
            for i in range(0, len(input_files), batch_size):
                end = min(i + batch_size, len(input_files))
                file_batches.append(input_files[i:end])
            
            print(f"Processing {len(input_files)} files in {len(file_batches)} batches using {num_processes} processes")
            
            # Process batches in parallel
            config_str = json.dumps(default_config()["data_writer"])
            process_func = partial(process_file_batch, temp_dir=temp_dir, config_str=config_str)
            
            temp_files = []
            
            with multiprocessing.Pool(processes=num_processes) as pool:
                results = list(tqdm.tqdm(pool.imap(process_func, file_batches), total=len(file_batches)))
                
                for temp_file in results:
                    if temp_file:
                        temp_files.append(temp_file)
            
            # Now merge all temporary files into the final output
            if temp_files:
                # Use the first temp file to initialize the final writer
                first_source = RootEventSource(temp_files[0])
                final_writer = DataWriter(first_source, config_str=json.dumps(default_config()["data_writer"]), filename=args.output)
                
                # Combine statistics from all files
                combined_statistics = Statistics()
                
                # Process all temp files
                for temp_file in temp_files:
                    source = RootEventSource(temp_file)
                    if source.statistics is not None:
                        combined_statistics += source.statistics
                    for event in source:
                        final_writer(event)
                
                final_writer.write_statistics(combined_statistics)
                final_writer.close()
                
                # Clean up temp files
                for temp_file in temp_files:
                    try:
                        os.remove(temp_file)
                    except:
                        pass
                os.rmdir(temp_dir)
                
        except Exception as e:
            print(f"Error in parallel processing: {e}")
            # Fall back to sequential processing
            args.jobs = 1
    
    # Sequential processing
    if args.jobs <= 1:
        statistics = Statistics()

        for i, input_file in enumerate(input_files):
            source = RootEventSource(input_file)
            if i == 0:
                data_writer = DataWriter(source, config_str=json.dumps(default_config()["data_writer"]), filename=args.output)
            if(source.statistics is not None):
                statistics += source.statistics
            for event in source:
                data_writer(event)
        data_writer.write_statistics(statistics)
        data_writer.close()
