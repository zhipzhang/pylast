"""
pytest tests for the awkward-array interface (get_ak_array).

Run after installing the package in a Python 3.13 environment:
    mamba activate test_awkward
    source /data/home/zzp/opt/root_install/bin/thisroot.sh
    pip install -v .
    pytest test/test_python_ak.py -v
"""

import os
import math
import pytest

awkward = pytest.importorskip("awkward")

from pylast._pylast_akarray import get_ak_array
from pylast._pyeventsource import RootEventSource, SimtelEventSource

# ---------------------------------------------------------------------------
# Paths to test data (relative to repo root; use absolute here for safety)
# ---------------------------------------------------------------------------
_REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_DATA = os.path.join(_REPO, "test", "test_data")

ROOT_FILE   = os.path.join(_DATA, "root_source_1.root")
SIMTEL_FILE = os.path.join(_DATA,
    "lact_prod0_simtel_particle_gamma_energy_1000.0_1000.0_"
    "zenith_0.0_azimuth_0.0_run_1_event_0.zst")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def _assert_ak_array_structure(events) -> None:
    """Basic structural checks that apply to any non-empty events array."""
    assert len(events) > 0, "Expected at least one event"

    for field in ("event_id", "run_id", "simulation", "pointing", "dl0", "dl1", "dl2"):
        assert field in events.fields, f"Missing top-level field: {field}"

    assert "int" in str(events.event_id.type), "event_id should be integer"


def _assert_simulation(events) -> None:
    """Checks for the simulation sub-record (should be present in sim files)."""
    sim = events.simulation
    valid_mask = ~awkward.is_none(sim)
    assert awkward.any(valid_mask), "No events with simulation data found"

    sim_valid = awkward.drop_none(sim)
    # Shower scalars
    for field in ("energy", "alt", "az", "core_x", "core_y",
                  "h_first_int", "x_max", "h_max", "starting_grammage",
                  "shower_primary_id"):
        assert field in sim_valid.fields, f"Missing simulation field: {field}"

    # New fields: triggered_tels list and tels sub-record
    assert "triggered_tels" in sim_valid.fields, "Missing simulation.triggered_tels"
    assert "tels" in sim_valid.fields, "Missing simulation.tels sub-record"

    energies = awkward.to_numpy(sim_valid.energy)
    assert all(e > 0 for e in energies), "All energies should be positive"


def _assert_pointing(events) -> None:
    """Checks for the pointing sub-record."""
    pt = events.pointing
    valid_mask = ~awkward.is_none(pt)
    if not awkward.any(valid_mask):
        return

    pt_valid = awkward.drop_none(pt)
    for field in ("array_azimuth", "array_altitude", "tel_ids",
                  "tel_azimuth", "tel_altitude"):
        assert field in pt_valid.fields, f"Missing pointing field: {field}"


def _assert_dl1(events) -> None:
    """Checks for the DL1 sub-record — full ImageParameters, no pixel arrays."""
    dl1 = events.dl1
    valid_mask = ~awkward.is_none(dl1)
    if not awkward.any(valid_mask):
        return

    dl1_valid = awkward.drop_none(dl1)
    # Top-level fields
    for field in ("tel_ids", "hillas", "leakage", "concentration",
                  "morphology", "intensity", "extra"):
        assert field in dl1_valid.fields, f"Missing DL1 field: {field}"

    # Per-pixel arrays must not be present (handled in C++)
    for field in ("image", "peak_time", "mask"):
        assert field not in dl1_valid.fields, \
            f"DL1 '{field}' should not be in ak.Array (too heavy, handle in C++)"

    # Hillas sub-fields
    for f in ("length", "width", "psi", "x", "y",
              "skewness", "kurtosis", "intensity", "r", "phi", "scale_ratio"):
        assert f in dl1_valid.hillas.fields, f"DL1 hillas missing '{f}'"
    # Leakage
    for f in ("pixels_width_1", "pixels_width_2",
              "intensity_width_1", "intensity_width_2"):
        assert f in dl1_valid.leakage.fields, f"DL1 leakage missing '{f}'"
    # Morphology
    for f in ("n_pixels", "n_islands",
              "n_small_islands", "n_medium_islands", "n_large_islands"):
        assert f in dl1_valid.morphology.fields, f"DL1 morphology missing '{f}'"

    # All per-tel lists must have the same count per event as tel_ids
    n_tels = awkward.num(dl1_valid.tel_ids, axis=1)
    assert awkward.num(dl1_valid.hillas.length,    axis=1).tolist() == n_tels.tolist()
    assert awkward.num(dl1_valid.hillas.intensity, axis=1).tolist() == n_tels.tolist()
    assert awkward.num(dl1_valid.leakage.pixels_width_1, axis=1).tolist() == n_tels.tolist()
    assert awkward.num(dl1_valid.morphology.n_pixels, axis=1).tolist() == n_tels.tolist()


def _assert_dl2_nested(dl2_valid) -> None:
    """Verify the nested DL2 sub-records (geometry, energy, particle, tels)."""
    assert "geometry" in dl2_valid.fields, "DL2 missing 'geometry' sub-record"
    assert "energy"   in dl2_valid.fields, "DL2 missing 'energy' sub-record"
    assert "particle" in dl2_valid.fields, "DL2 missing 'particle' sub-record"
    assert "tels"     in dl2_valid.fields, "DL2 missing 'tels' sub-record"

    geom = dl2_valid.geometry
    for f in ("methods", "is_valid", "alt", "az",
              "core_x", "core_y", "hmax", "xmax", "tel_ids"):
        assert f in geom.fields, f"DL2 geometry missing field: {f}"

    en = dl2_valid.energy
    for f in ("methods", "energy_valid", "estimate", "estimate_std", "tel_ids"):
        assert f in en.fields, f"DL2 energy missing field: {f}"

    pt = dl2_valid.particle
    for f in ("methods", "is_valid", "hadroness", "mrsl", "mrsw", "tel_ids"):
        assert f in pt.fields, f"DL2 particle missing field: {f}"

    tels = dl2_valid.tels
    for f in ("ids", "estimate_energy", "estimate_hadroness", "estimate_disp",
              "impact_distance", "impact_distance_error"):
        assert f in tels.fields, f"DL2 tels missing field: {f}"


# ---------------------------------------------------------------------------
# Tests: RootEventSource
# ---------------------------------------------------------------------------
class TestRootEventSource:
    @pytest.fixture(scope="class")
    def events(self):
        if not os.path.exists(ROOT_FILE):
            pytest.skip(f"Test data not found: {ROOT_FILE}")
        source = RootEventSource(ROOT_FILE, max_events=50)
        return get_ak_array(source)

    def test_structure(self, events):
        _assert_ak_array_structure(events)

    def test_event_ids_present(self, events):
        """event_id values should be non-negative integers."""
        ids = awkward.to_numpy(events.event_id)
        assert all(i >= 0 for i in ids.tolist())

    def test_simulation_fields(self, events):
        _assert_simulation(events)

    def test_simulation_triggered_tels(self, events):
        """triggered_tels should be a ragged list of tel IDs per event."""
        sim_valid = awkward.drop_none(events.simulation)
        if len(sim_valid) == 0:
            pytest.skip("No simulation data")
        trig = sim_valid.triggered_tels
        # Each event has at most one list of triggered telescopes (possibly empty)
        counts = awkward.num(trig, axis=1)
        assert awkward.all(counts >= 0), "triggered_tels counts must be non-negative"

    def test_simulation_tels_sub_record(self, events):
        """simulation.tels: scalars + full ImageParameters sub-records."""
        sim_valid = awkward.drop_none(events.simulation)
        if len(sim_valid) == 0:
            pytest.skip("No simulation data")
        sim_tels = sim_valid.tels
        for f in ("ids", "true_image_sum", "impact_parameter", "time_range_10_90",
                  "hillas", "leakage", "concentration", "morphology",
                  "intensity", "extra"):
            assert f in sim_tels.fields, f"simulation.tels missing field: {f}"

        # Per-pixel images must not be present (handled in C++)
        for f in ("true_image", "fake_image"):
            assert f not in sim_tels.fields, \
                f"simulation.tels.{f} should not be in ak.Array (handle in C++)"

        # All per-tel lists must share the same per-event count
        n_ids = awkward.num(sim_tels.ids, axis=1)
        for arr in (sim_tels.true_image_sum, sim_tels.hillas.intensity,
                    sim_tels.leakage.pixels_width_1, sim_tels.morphology.n_pixels):
            assert awkward.num(arr, axis=1).tolist() == n_ids.tolist()

    def test_pointing_fields(self, events):
        _assert_pointing(events)

    def test_dl1_structure(self, events):
        _assert_dl1(events)

    def test_dl1_imgparam_shape(self, events):
        """All ImageParameters lists must have the same per-event telescope count."""
        dl1 = awkward.drop_none(events.dl1)
        if len(dl1) == 0:
            pytest.skip("No DL1 data in this file")
        n_tels = awkward.num(dl1.tel_ids, axis=1).tolist()
        checks = [
            dl1.hillas.length, dl1.hillas.intensity,
            dl1.leakage.pixels_width_1,
            dl1.concentration.cog,
            dl1.morphology.n_pixels,
            dl1.intensity.max,
            dl1.extra.miss,
        ]
        for arr in checks:
            assert awkward.num(arr, axis=1).tolist() == n_tels

    def test_dl2_nested_structure(self, events):
        """DL2 must have nested geometry/energy/particle/tels sub-records."""
        dl2 = events.dl2
        valid = ~awkward.is_none(dl2)
        if not awkward.any(valid):
            pytest.skip("No DL2 data in this ROOT file")
        _assert_dl2_nested(awkward.drop_none(dl2))

    def test_dl2_geometry_method_names(self, events):
        """geometry.methods should be a ragged list of strings per event."""
        dl2_valid = awkward.drop_none(events.dl2)
        if len(dl2_valid) == 0:
            pytest.skip("No DL2 data")
        n_methods = awkward.num(dl2_valid.geometry.methods, axis=1)
        n_alt     = awkward.num(dl2_valid.geometry.alt,     axis=1)
        assert n_methods.tolist() == n_alt.tolist(), \
            "geometry.methods and geometry.alt must have the same per-event length"

    def test_dl2_impact_shape(self, events):
        """tels.impact_distance shape must be [n_tels, n_geom_methods]."""
        dl2_valid = awkward.drop_none(events.dl2)
        if len(dl2_valid) == 0:
            pytest.skip("No DL2 data")
        n_tels         = awkward.num(dl2_valid.tels.ids,             axis=1)
        n_impact_outer = awkward.num(dl2_valid.tels.impact_distance,  axis=1)
        assert n_tels.tolist() == n_impact_outer.tolist(), \
            "tels.impact_distance outer dimension must match tels.ids length"

    def test_vectorized_energy(self, events):
        """Vectorized access to simulation.energy should return numpy array."""
        import numpy as np
        sim_valid = awkward.drop_none(events.simulation)
        if len(sim_valid) == 0:
            pytest.skip("No simulation data")
        energies = awkward.to_numpy(sim_valid.energy)
        assert isinstance(energies, np.ndarray)
        assert energies.ndim == 1


# ---------------------------------------------------------------------------
# Tests: SimtelEventSource
# ---------------------------------------------------------------------------
class TestSimtelEventSource:
    @pytest.fixture(scope="class")
    def events(self):
        if not os.path.exists(SIMTEL_FILE):
            pytest.skip(f"Test data not found: {SIMTEL_FILE}")
        source = SimtelEventSource(SIMTEL_FILE, max_events=20)
        return get_ak_array(source)

    def test_structure(self, events):
        _assert_ak_array_structure(events)

    def test_simulation_fields(self, events):
        _assert_simulation(events)

    def test_simulation_tels_camera(self, events):
        """Simtel should populate SimulatedCamera scalar data in simulation.tels."""
        sim_valid = awkward.drop_none(events.simulation)
        if len(sim_valid) == 0:
            pytest.skip("No simulation data")
        sim_tels = sim_valid.tels
        n_tels = awkward.num(sim_tels.ids, axis=1)
        if not awkward.any(n_tels > 0):
            pytest.skip("No per-telescope camera data in this simtel file")
        # true_image_sum should be non-negative integers
        sums = awkward.flatten(sim_tels.true_image_sum)
        assert awkward.all(sums >= 0), "true_image_sum must be non-negative"

    def test_pointing_fields(self, events):
        _assert_pointing(events)

    def test_dl0_structure(self, events):
        """SimtelEventSource should provide DL0 (raw waveform sums)."""
        dl0 = events.dl0
        valid = ~awkward.is_none(dl0)
        if not awkward.any(valid):
            pytest.skip("No DL0 data in this simtel file")
        dl0_valid = awkward.drop_none(dl0)
        for field in ("tel_ids", "image", "peak_time"):
            assert field in dl0_valid.fields, f"Missing DL0 field: {field}"

    def test_primary_particle(self, events):
        """All events from a single-particle simtel file should have the same primary."""
        sim_valid = awkward.drop_none(events.simulation)
        if len(sim_valid) == 0:
            pytest.skip("No simulation data")
        primary_ids = awkward.to_numpy(sim_valid.shower_primary_id)
        assert len(set(primary_ids.tolist())) == 1, \
            "Single-particle file should have uniform shower_primary_id"

    def test_simulation_energy_range(self, events):
        """Test file is fixed-energy gamma: energies should be ~1000 GeV = 1 TeV."""
        sim_valid = awkward.drop_none(events.simulation)
        if len(sim_valid) == 0:
            pytest.skip("No simulation data")
        energies = awkward.to_numpy(sim_valid.energy)
        assert all(math.isclose(e, 1.0, rel_tol=0.01) for e in energies), \
            "Expected ~1 TeV energies in fixed-energy gamma file"
