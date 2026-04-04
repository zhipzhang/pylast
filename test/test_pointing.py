import pytest
from pathlib import Path
from astropy.time import Time
from pylast.pointing.pointing_data import PointingData

@pytest.fixture(scope="module")
def pointing_file():
    return Path(__file__).parent / "test_data" / "test_pointing.txt"

@pytest.fixture(scope="module")
def pdata(pointing_file):
    return PointingData(str(pointing_file))

class TestPointingLoading:
    """Tests for PointingData initialization and basic properties"""
    
    def test_time_bounds(self, pdata):
        assert pdata.start_time.mjd == pytest.approx(61110.5, abs=1e-9)
        assert pdata.end_time.mjd == pytest.approx(61110.50011, abs=1e-9)
    
    def test_mjd_float_range(self, pdata):
        assert pdata.mjd_float.min() == pytest.approx(61110.5, abs=1e-9)
        assert pdata.mjd_float.max() == pytest.approx(61110.50011, abs=1e-9)
    
    def test_azimuth_altitude_bounds(self, pdata):
        assert pdata.data["azimuth"].min() == pytest.approx(10.0, abs=1e-3)
        assert pdata.data["altitude"].max() == pytest.approx(0.0, abs=1e-3)

class TestPointingInterpolation:
    """Tests for coordinate interpolation - critical for IACT analysis"""
    
    @pytest.mark.parametrize("mjd, exp_az, exp_alt", [
        (61110.50002, 25.0, 0.0),
        (61110.5, 10.0, 0.0),
    ])
    def test_interpolation_accuracy(self, pdata, mjd, exp_az, exp_alt):
        az, alt = pdata.interpolate_pointing(mjd)
        assert az == pytest.approx(exp_az, abs=0.01)  # 36 arcsec tolerance
        assert alt == pytest.approx(exp_alt, abs=0.01)
    def test_interpolation_range(self, pdata):
        with pytest.raises(ValueError):
            pdata.interpolate_pointing(61110.49999)
        with pytest.raises(ValueError):
            pdata.interpolate_pointing(61110.50012)