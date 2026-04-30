from pylast.pointing import PointingData
from pylast.io import PrototypeEventSource, DataWriter, RootEventSource
from pylast.helper import PointingTelescope
from pylast.calib import PrototypeCalibrator
import argparse
import json
import numpy as np


def main():
    parser = argparse.ArgumentParser(description='Generate prototype event source')
    parser.add_argument('--input', '-i', required=True, help='Input file path')
    parser.add_argument(
        '--pointing_file', '-p', 
        required=True, 
        nargs='+',
        help='Pointing file path(s), space-separated for multiple files'
    )
    parser.add_argument('--output', '-o', required=True, help='Output file path')
    parser.add_argument('--config', '-c', required=True, help='Config file path for json')
    parser.add_argument('--exclude-led', action='store_true', help='Exclude LED events')
    parser.add_argument('--only-led', action='store_true', help='Only include LED events')
    parser.add_argument('--only-on', action='store_true', help='Only include events when pointing is on')
    parser.add_argument('--only-off', action='store_true', help='Only include events when pointing is off')
    args = parser.parse_args()

    # args.pointing_file is a list of one or more file paths
    pointing_data = PointingData(args.pointing_file)
    prototype_event_source = PrototypeEventSource(args.input)
    config = None
    if args.config is not None:
        config = json.load(open(args.config))
    if config is None:
        raise ValueError("Config file is not provided")
    data_writer = DataWriter(source=prototype_event_source, filename=args.output, config_str=json.dumps(config["data_writer"]))
    prototype_calibrator = PrototypeCalibrator(config_str=json.dumps(config["prototype_calibrator"]))

    if args.exclude_led and args.only_led:
        raise ValueError("Cannot exclude and only include LED events at the same time")
    exclude_led = False
    only_led = False
    if args.exclude_led:
        exclude_led = True
    if args.only_led:
        only_led = True
    with data_writer as writer:
        for event in prototype_event_source:
            prototype_calibrator(event)
            low_gain_mean_peak = np.mean(event.c1.tels[1].low_gain_peak)
            if exclude_led:
                if low_gain_mean_peak > 140:
                    continue
            if only_led:
                if low_gain_mean_peak < 140:
                    continue
            if args.only_on:
                if not pointing_data.is_on(event.mjd.to_float()):
                    continue
            if args.only_off:
                if not pointing_data.is_off(event.mjd.to_float()):
                    continue
            azimuth, altitude = pointing_data.interpolate_pointing(event.mjd.to_float())
            event.pointing.array_azimuth = azimuth
            event.pointing.array_altitude = altitude
            event.pointing.add_tel(1, PointingTelescope(azimuth, altitude))
            writer(event)

if __name__ == "__main__":
    main()
