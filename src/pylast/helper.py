from ._pylast_arrayevent import *
from ._pylast_subarray import *
from ._pyeventsource import *
from ._pylast_calibrator import *
from ._pylast_imageprocessor import *
from ._pylast_showerprocessor import *
from ._pylast_datawriter import *
from ._pystatistic import *
import numpy as np

import numba as nb


@nb.njit
def compute_angle_separation(rec_alt, rec_az, true_alt, true_az):
    sin_alt1 = np.sin(rec_alt)
    sin_alt2 = np.sin(true_alt)
    cos_alt1 = np.cos(rec_alt)
    cos_alt2 = np.cos(true_alt)
    cos_az_diff = np.cos(rec_az - true_az)
    cos_angle = sin_alt1 * sin_alt2 + cos_alt1 * cos_alt2 * cos_az_diff
    # Replace np.clip with manual min/max operations for scalar values
    if cos_angle < -1.0:
        cos_angle = -1.0
    elif cos_angle > 1.0:
        cos_angle = 1.0
    return np.degrees(np.arccos(cos_angle))
