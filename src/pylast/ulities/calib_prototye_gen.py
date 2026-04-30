import numpy as np
import argparse

import json
from pylast.io import RootEventSource, DataWriter
from pylast.helper import DL0Camera, DL0Event
from pylast.image import ImageProcessor
from pylast.calib import PrototypeCalibrator

LASER_TIME = [833200, 432100, 131200]

def is_laser_event(event) -> bool:
    rabbitTime, rabbittime = event.mjd.to_rabbit_time()
    event_ns = rabbittime * 8
    return any(abs(event_ns - t) < 20000 for t in LASER_TIME)


def main():
    parser = argparse.ArgumentParser(description="Calibrate and compute Hillas parameters")
    parser.add_argument("--input", "-i", required=True, help="Input file path")
    parser.add_argument("--output", "-o", required=True, help="Output file path")
    parser.add_argument("--config", "-c", required=True, help="Config file path")
    parser.add_argument("--led-event-file", required=True, help="LED event file path")
    args = parser.parse_args()

    conversion_factors = None
    normal_pixels = None

    config = json.load(open(args.config))
    prototype_calibrator = PrototypeCalibrator(config_str=json.dumps(config["prototype_calibrator"]))

    with open(args.led_event_file, 'r') as f:
        led_event_file_list = f.read().splitlines()
    low_gain_area_row = []
    for led_event_file in led_event_file_list:
        source = RootEventSource(led_event_file)
        for event in source:
            prototype_calibrator.advanced_process(event)
            low_gain_area_row.append(np.copy(event.c1.tels[1].low_gain_area))
        del source
    X = np.stack(low_gain_area_row, axis = 0)
    low_gain_per_pixel = X.T
    pixel_mean_area = np.nanmean(low_gain_per_pixel, axis = 1)
    mean_area = np.nanmean(pixel_mean_area)
    std_area = np.nanstd(pixel_mean_area)
    normal_pixels = np.fabs(pixel_mean_area - mean_area) < 4 * std_area
    conversion_factors = 2800 / pixel_mean_area

    del low_gain_area_row, X
    source = RootEventSource(args.input)

    image_processor = ImageProcessor(source.subarray, json.dumps(config["image_processor"]))
    data_writer = DataWriter(source, args.output, json.dumps(config["data_writer"]))
    for event in source:
        if is_laser_event(event):
            continue
        prototype_calibrator.advanced_process(event)
        pe = event.c1.tels[1].low_gain_area * conversion_factors - 5
        pe = np.where(normal_pixels, pe, -999)
        peak_time = event.c1.tels[1].low_gain_peak_time
        event.initialize_dl0()
        event.dl0.add_tel(1, DL0Camera(pe, peak_time))
        image_processor(event)
        data_writer(event)
if __name__ == "__main__":
    main()

