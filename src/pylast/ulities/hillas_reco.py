#!/usr/bin/env python3
import statistics
from ..helper import register_exe
import sys
import argparse
from pathlib import Path


def _plot_requested(args):
    return bool(getattr(args, "plot_dir", None))


def _make_event_plotter(source, args):
    if not _plot_requested(args):
        return None, None

    from pylast.visualize import EventVisualizer

    plot_dir = Path(args.plot_dir)
    plot_dir.mkdir(parents=True, exist_ok=True)
    visualizer = EventVisualizer(
        source,
        enable_secondary_axes=not args.plot_no_secondary_axes,
        outline_pixels=not args.plot_no_pixel_edges,
    )
    return visualizer, plot_dir


def _maybe_plot_event(visualizer, plot_dir, event, event_index, input_index, plot_count, args):
    if visualizer is None:
        return False
    if args.plot_max is not None and plot_count >= args.plot_max:
        return False
    if args.plot_every <= 0:
        return False
    if event_index % args.plot_every != 0:
        return False

    event_id = getattr(event, "event_id", event_index)
    output_path = plot_dir / f"input{input_index:03d}_event{event_id}_{args.plot_kind}_{args.plot_level}.png"

    if args.plot_kind == "telescopes":
        visualizer.plot_telescopes(
            event,
            output_path=str(output_path),
            image_level=args.plot_level,
            show=False,
        )
    elif args.plot_kind == "gathered":
        visualizer.plot_gathered_event(
            event,
            output_path=str(output_path),
            image_level=args.plot_level,
            show_hillas=args.plot_hillas,
            show_ideal_position=args.plot_ideal_position,
            show=False,
        )
    else:
        visualizer.plot_event(
            event,
            output_path=str(output_path),
            image_level=args.plot_level,
            show_hillas=args.plot_hillas,
            show_ideal_position=args.plot_ideal_position,
            show=False,
        )
    return True

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
    for input_index, (input_fname, output_fname) in enumerate(zip(input_files, output_files)):
        print(f"Processing: {input_fname} -> {output_fname}")
        if args.subarray:
            source = SimtelEventSource(input_fname, -1, [int(tel_id) for tel_id in args.subarray.split(',')])
        else:
            source = SimtelEventSource(input_fname)
        calibrator = Calibrator(source.subarray, config_str=json.dumps(config.get("calibrator", {})))
        image_processor = ImageProcessor(source.subarray, config_str=json.dumps(config.get("image_processor", {})))
        shower_processor = ShowerProcessor(source.subarray, config_str=json.dumps(config.get("shower_processor", {})))
        data_writer = DataWriter(source, output_fname, config_str=json.dumps(config.get("data_writer", {})))
        visualizer, plot_dir = _make_event_plotter(source, args)
        
        plot_count = 0
        for event_index, event in enumerate(source):
            calibrator(event)
            image_processor(event)
            shower_processor(event)
            if _maybe_plot_event(visualizer, plot_dir, event, event_index, input_index, plot_count, args):
                plot_count += 1
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
    parser.add_argument('--plot-dir',
                       help='Directory for event-check plots. Enables the Python enhanced processing loop.')
    parser.add_argument('--plot-every', type=int, default=1,
                       help='Plot every N processed events when --plot-dir is set (default: 1)')
    parser.add_argument('--plot-max', type=int,
                       help='Maximum number of plots to write per input file')
    parser.add_argument('--plot-level', choices=['simulation', 'dl0', 'dl1'], default='dl1',
                       help='Image level to plot after processing (default: dl1)')
    parser.add_argument('--plot-kind', choices=['event', 'gathered', 'telescopes'], default='event',
                       help='Plot type to write (default: event)')
    parser.add_argument('--plot-hillas', action='store_true',
                       help='Overlay Hillas ellipses when plotting camera images')
    parser.add_argument('--plot-ideal-position', action='store_true',
                       help='Overlay the true source position projected to the camera when pointing is available')
    parser.add_argument('--plot-no-secondary-axes', action='store_true',
                       help='Disable camera degree secondary axes for faster plotting')
    parser.add_argument('--plot-no-pixel-edges', action='store_true',
                       help='Disable pixel outlines for faster plotting')
    
    # Parse all arguments
    args = parser.parse_args()
    
    if args.enhanced or _plot_requested(args):
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
