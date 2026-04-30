import pandas as pd
from astropy.time import Time, TimeDelta, TimezoneInfo
import fsspec
import fsspec_xrootd
import astropy.units as u
import numpy as np
from typing import List


class PointingData:
    def __init__(self, file_path: str | List[str]):
        self.file_paths = []
        if isinstance(file_path, list):
            for path in file_path:
                if path.startswith("/eos"):
                    path = "root://eos01.ihep.ac.cn/" + path
                self.file_paths.append(path)
        else:
            if file_path.startswith("/eos"):
                file_path = "root://eos01.ihep.ac.cn/" + file_path
            self.file_paths.append(file_path)
        self.data = pd.DataFrame()
        for file in self.file_paths:
            with fsspec.open(file, mode="r") as file_obj:
                data = pd.read_csv(
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
                self.data = pd.concat([self.data, data])
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

    def _is_status(self, time, status_code: str):
        """
        Checks if the pointing status matches the given status_code at given time(s).

        Parameters
        ----------
        time : float or array-like
            MJD value(s)
        status_code : str
            The status code to check (e.g., "01", "02")

        Returns
        -------
        bool or np.ndarray
            True if status matches status_code at the given time(s), False otherwise.
        """
        mjd_min = self.mjd_float.min()
        mjd_max = self.mjd_float.max()

        if isinstance(time, float):
            if time < mjd_min or time > mjd_max:
                raise ValueError(f"time ({time}) is out of range {mjd_min} to {mjd_max}")
            idx = (np.abs(self.mjd_float - time)).argmin()
            status = self.data.iloc[idx]["status"]
            return str(status).strip() == status_code
        elif isinstance(time, (np.ndarray, list)):
            time = np.asarray(time, dtype=float)
            if np.any(time < mjd_min) or np.any(time > mjd_max):
                raise ValueError("Some values in time are out of range "
                                 f"{mjd_min} to {mjd_max}")
            output = []
            for t in time:
                idx = (np.abs(self.mjd_float - t)).argmin()
                status = self.data.iloc[idx]["status"]
                output.append(str(status).strip() == status_code)
            return np.array(output)
        else:
            raise TypeError("time should be a float (MJD) or a list/array of floats (MJD)")

    def is_on(self, time) -> bool:
        """
        Checks if the pointing status is "on" ("01") at given time(s).
        """
        return self._is_status(time, "01")

    def is_off(self, time) -> bool:
        """
        Checks if the pointing status is "off" ("02") at given time(s).
        """
        return self._is_status(time, "02")
       

    def interpolate_pointing(self, time):
        # Interpolates azimuth and altitude for a given time (float MJD), using numpy for 1d interpolation
        # time: float, scalar or array (MJD)
        # returns: dict with interpolated azimuth and altitude

        if isinstance(time, float):
            if time < self.mjd_float.min() or time > self.mjd_float.max():
                raise ValueError(
                    f"Time {time} is out of range {self.mjd_float.min()} to {self.mjd_float.max()}"
                )

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
