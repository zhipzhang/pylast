#!/usr/bin/env python3
import statistics
from ..helper import register_exe
import sys
import argparse

def enhanced_hillas_reco(args):
    """
    Enhanced Hillas reconstruction implementation.
    Add your custom implementation here.
    
    Args:
        args: Parsed command line arguments
    """
    print("Running enhanced Hillas reconstruction...")
    
    # Handle multiple input and output files
    input_files = args.input if isinstance(args.input, list) else [args.input]
    output_files = args.output if isinstance(args.output, list) else [args.output]
    
    # Assert that the number of input and output files match
    assert len(input_files) == len(output_files), f"Number of input files ({len(input_files)}) must match number of output files ({len(output_files)})"
    
    print(f"Processing {len(input_files)} file pairs:")
    for i, (input_file, output_file) in enumerate(zip(input_files, output_files)):
        print(f"  {i+1}: {input_file} -> {output_file}")
    
    from pylast.io import SimtelEventSource,DataWriter
    from pylast.reco import ShowerProcessor
    from pylast.calib import Calibrator
    from pylast.image import ImageProcessor
    import json
    from pylast.helper import Statistics
    
    config = json.load(open(args.config))
    
    # Process each input-output file pair
    for input_fname, output_fname in zip(input_files, output_files):
        print(f"Processing: {input_fname} -> {output_fname}")
        if args.subarray:
            source = SimtelEventSource(input_fname, -1, [int(tel_id) for tel_id in args.subarray.split(',')])
        else:
            source = SimtelEventSource(input_fname)
        calibrator = Calibrator(source.subarray, config_str=json.dumps(config.get("calibrator", {})))
        image_processor = ImageProcessor(source.subarray, config_str=json.dumps(config.get("image_processor", {})))
        shower_processor = ShowerProcessor(source.subarray, config_str=json.dumps(config.get("shower_processor", {})))
        data_writer = DataWriter(source, output_fname, config_str=json.dumps(config.get("data_writer", {})))
        
        for event in source:
            calibrator(event)
            image_processor(event)
            shower_processor(event)
            data_writer(event)

        data_writer.write_all_simulation_shower(source.shower_array)
        data_writer.close()
        del source, data_writer, calibrator, shower_processor, image_processor
    
    return 0

def main():
    parser = argparse.ArgumentParser(description='Hillas reconstruction tool')
    parser.add_argument('--enhanced', action='store_true', 
                       help='Use enhanced Hillas reconstruction implementation')
    
    # Add common arguments that the hillas_reco executable typically uses
    parser.add_argument('-i', '--input', required=True, action='append',
                       help='Input file path (can be specified multiple times)')
    parser.add_argument('-o', '--output', required=True, action='append',
                       help='Output file path (can be specified multiple times)')
    parser.add_argument('-c', '--config', 
                       help='Configuration file path')
    parser.add_argument('-s', '--subarray',
                        help='Specify telescopes to use (comma-separated list, e.g., "1,2,3,4")')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Verbose output')
    
    # Parse all arguments
    args = parser.parse_args()
    
    if args.enhanced:
        return enhanced_hillas_reco(args)
    else:
        # Use the default executable - pass all args except --enhanced
        original_argv = sys.argv.copy()
        if '--enhanced' in sys.argv:
            sys.argv.remove('--enhanced')
        try:
            return register_exe("hillas_reco")
        finally:
            # Restore original argv
            sys.argv = original_argv

if __name__ == "__main__":
    sys.exit(main())