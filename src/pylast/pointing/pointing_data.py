import pandas as pd
from astropy.time import Time, TimeDelta, TimezoneInfo
import fsspec
import fsspec_xrootd
import astropy.units as u
import numpy as np


class PointingData:
    def __init__(self, file_path: str):
        if file_path.startswith("/eos"):
            file_path = "root://eos01.ihep.ac.cn/" + file_path
        self.file_path = file_path
        with fsspec.open(file_path, mode="r") as file_obj:
            self.data = pd.read_csv(
                file_obj,
                sep=r"\s+",
                header=None,
                skiprows=1,
                names=["date", "time", "azimuth", "altitude", "mjd_raw", "status"],
                dtype={
                    "date": str,
                    "time": str,
                    "azimuth": float,
                    "altitude": float,
                    "mjd_raw": str,
                    "status": str,
                },
            )

        day = np.array([s.split(".")[0] for s in self.data["mjd_raw"]], dtype=np.int64)
        frac = np.array(
            ["0." + s.split(".")[1] for s in self.data["mjd_raw"]], dtype=np.float64
        )
        self.data["mjd"] = Time(day, format="mjd") + TimeDelta(frac, format="jd")
        self.start_time = self.data["mjd"].min()
        self.end_time = self.data["mjd"].max()
        self.mjd_float = self.data["mjd"].apply(lambda x: x.mjd)

    def __repr__(self):
        # Convert start_time and end_time to datetime with UTC+8 timezone
        beijing_timezone = TimezoneInfo(utc_offset=8 * u.hour)
        start_dt = self.start_time.to_datetime(timezone=beijing_timezone)
        end_dt = self.end_time.to_datetime(timezone=beijing_timezone)
        return f"PointingData: {self.file_path} from {start_dt} to {end_dt}"

    def interpolate_pointing(self, time):
        # Interpolates azimuth and altitude for a given time (float MJD), using numpy for 1d interpolation
        # time: float, scalar or array (MJD)
        # returns: dict with interpolated azimuth and altitude

        if isinstance(time, float):
            if time < self.mjd_float.min() or time > self.mjd_float.max():
                raise ValueError("Time is out of range")

            az_interp = np.interp(time, self.mjd_float, self.data["azimuth"])
            alt_interp = np.interp(time, self.mjd_float, self.data["altitude"])
            return az_interp, alt_interp

        elif isinstance(time, (np.ndarray, list)):
            time = np.asarray(time, dtype=float)
            if np.any(time < self.mjd_float.min()) or np.any(
                time > self.mjd_float.max()
            ):
                raise ValueError("Some times are out of range")

            az_interp = np.interp(time, self.mjd_float, self.data["azimuth"])
            alt_interp = np.interp(time, self.mjd_float, self.data["altitude"])
            return az_interp, alt_interp
        else:
            raise TypeError(
                "time should be a float (MJD) or a list/array of floats (MJD)"
            )
