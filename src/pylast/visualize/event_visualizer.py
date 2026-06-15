"""
Event visualization primitives for pylast.

This module is based on the LACT event plotting workflow, but is written as a
package module: no local paths, no notebook-only state, and no hard dependency
on a specific input file. It focuses on camera, array, and SDP diagnostics for
already loaded pylast event containers.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, Iterable, Mapping, Optional, Sequence, Tuple

import matplotlib
import matplotlib.colors as mcolors
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.collections import PolyCollection
from matplotlib.colors import Normalize
from matplotlib.patches import Ellipse
from matplotlib.ticker import FormatStrFormatter, MaxNLocator
from mpl_toolkits.axes_grid1 import make_axes_locatable

from .lhaaso_background import draw_lhaaso_background


matplotlib.rcParams["path.simplify"] = True
matplotlib.rcParams["agg.path.chunksize"] = 20000


_ROOT_TRIGGERED_CACHE: Dict[Tuple[str, int], Tuple[int, ...]] = {}
_ROOT_POINTING_CACHE: Dict[str, Optional[float]] = {}


@dataclass
class TelescopeGeometry:
    tel_id: int
    pos_x: float
    pos_y: float
    focal_length: float
    pix_x: np.ndarray
    pix_y: np.ndarray
    pix_size: np.ndarray


@dataclass
class EventData:
    event_id: int
    energy: float
    core_x: float
    core_y: float
    zenith_deg: float
    azimuth_deg: float
    x_max: float
    first_interaction_height: float
    image_by_tel: Dict[int, np.ndarray]
    image_sum_by_tel: Dict[int, float]
    active_tels: np.ndarray


@dataclass
class HillasParameters:
    length: float
    width: float
    psi: float
    cog_x: float
    cog_y: float


def _to_numpy(value, dtype=float) -> np.ndarray:
    return np.asarray(value, dtype=dtype)


def _first_attr(obj, *names):
    for name in names:
        if hasattr(obj, name):
            return getattr(obj, name)
    raise AttributeError(f"{type(obj).__name__} has none of: {', '.join(names)}")


def _rad(deg: float) -> float:
    return np.deg2rad(deg)


def _deg(rad: float) -> float:
    return np.rad2deg(rad)


def _azimuth_vector_xy(azimuth_deg: float, mode: str = "source"):
    azimuth_rad = np.deg2rad(azimuth_deg)
    dx, dy = np.sin(azimuth_rad), np.cos(azimuth_rad)
    if mode == "incoming":
        return -dx, -dy
    return dx, dy


def _add_array_compass(ax, x0: float, y0: float, length: float):
    ax.annotate(
        "",
        xy=(x0, y0 + length),
        xytext=(x0, y0),
        arrowprops=dict(arrowstyle="-|>", lw=1.2, color="0.08"),
        zorder=8,
    )
    ax.text(x0, y0 + length * 1.12, "N", ha="center", va="bottom", fontsize=11, weight="bold")
    ax.text(x0, y0 - length * 0.18, "S", ha="center", va="top", fontsize=9, color="0.35")
    ax.annotate(
        "",
        xy=(x0 + length, y0),
        xytext=(x0, y0),
        arrowprops=dict(arrowstyle="-|>", lw=1.0, color="0.35"),
        zorder=8,
    )
    ax.text(x0 + length * 1.12, y0, "E", ha="left", va="center", fontsize=9, color="0.35")
    ax.text(x0 - length * 0.18, y0, "W", ha="right", va="center", fontsize=9, color="0.35")


def _add_telescope_direction_inset(ax, x0: float, y0: float, length: float, azimuth_deg: float):
    dx, dy = _azimuth_vector_xy(azimuth_deg, mode="source")
    end_x = x0 + dx * 0.72 * length
    end_y = y0 + dy * 0.72 * length
    ax.annotate(
        "",
        xy=(end_x, end_y),
        xytext=(x0, y0),
        arrowprops=dict(arrowstyle="-|>", lw=1.35, color="#2166ac"),
        zorder=8,
    )
    ax.text(
        end_x,
        end_y,
        "Telescope",
        ha="left" if dx >= 0 else "right",
        va="bottom" if dy >= 0 else "top",
        fontsize=8.0,
        color="#2166ac",
        weight="bold",
        bbox=dict(boxstyle="round,pad=0.12", facecolor="white", edgecolor="none", alpha=0.78),
        zorder=9,
    )


def _add_arrival_arrow(ax, core_x: float, core_y: float, azimuth_deg: float, span: float, mode: str = "incoming"):
    dx, dy = _azimuth_vector_xy(azimuth_deg, mode=mode)
    length = 0.13 * span
    start_x = core_x - dx * length
    start_y = core_y - dy * length
    label_x = start_x - dx * length * 0.07
    label_y = start_y - dy * length * 0.07
    ax.annotate(
        "",
        xy=(core_x, core_y),
        xytext=(start_x, start_y),
        arrowprops=dict(arrowstyle="-|>", lw=1.6, color="#b2182b"),
        zorder=8,
    )
    ax.text(
        label_x,
        label_y,
        "Event",
        ha="right" if dx >= 0 else "left",
        va="top" if dy >= 0 else "bottom",
        fontsize=8.2,
        color="#b2182b",
        weight="bold",
        zorder=9,
        bbox=dict(boxstyle="round,pad=0.16", facecolor="white", edgecolor="none", alpha=0.78),
    )


def _total_quantity_label(quantity: str) -> str:
    if quantity == "pe":
        return "Total p.e."
    return f"Total {quantity}"


def _event_info_text(event_id: int, core_x: Optional[float], core_y: Optional[float], arrival_az: Optional[float]) -> str:
    lines = []
    if event_id is not None:
        lines.append(f"event_id = {event_id}")
    if core_x is not None and core_y is not None:
        lines.append(f"core: x = {core_x:.1f} m, y = {core_y:.1f} m")
    if arrival_az is not None:
        lines.append(f"event az = {arrival_az:.2f} deg")
    return "\n".join(lines)


def _root_filename(source) -> Optional[str]:
    filename = getattr(source, "input_filename", None)
    if filename is None:
        return None
    return str(filename)


def _root_pointing_azimuth_deg(source) -> Optional[float]:
    filename = _root_filename(source)
    if filename is None:
        return None
    if filename in _ROOT_POINTING_CACHE:
        return _ROOT_POINTING_CACHE[filename]
    try:
        import ROOT

        root_file = ROOT.TFile.Open(filename)
        if not root_file or root_file.IsZombie():
            _ROOT_POINTING_CACHE[filename] = None
            return None
        tree = root_file.Get("telescopes")
        if tree is None or tree.GetEntries() == 0 or tree.GetBranch("pointing_az_deg") is None:
            root_file.Close()
            _ROOT_POINTING_CACHE[filename] = None
            return None
        tree.GetEntry(0)
        azimuth = float(tree.pointing_az_deg)
        root_file.Close()
    except Exception:
        azimuth = None
    _ROOT_POINTING_CACHE[filename] = azimuth
    return azimuth


def _root_triggered_tel_ids(source, event_id: int) -> np.ndarray:
    filename = _root_filename(source)
    if filename is None:
        return np.asarray([], dtype=int)
    cache_key = (filename, int(event_id))
    if cache_key in _ROOT_TRIGGERED_CACHE:
        return np.asarray(_ROOT_TRIGGERED_CACHE[cache_key], dtype=int)
    triggered_tel_ids = []
    try:
        import ROOT

        root_file = ROOT.TFile.Open(filename)
        if not root_file or root_file.IsZombie():
            _ROOT_TRIGGERED_CACHE[cache_key] = ()
            return np.asarray([], dtype=int)
        tree = root_file.Get("observations")
        if (
            tree is None
            or tree.GetBranch("event_id") is None
            or tree.GetBranch("telescope_id") is None
            or tree.GetBranch("triggered") is None
        ):
            root_file.Close()
            _ROOT_TRIGGERED_CACHE[cache_key] = ()
            return np.asarray([], dtype=int)
        for entry in range(tree.GetEntries()):
            tree.GetEntry(entry)
            if int(tree.event_id) == int(event_id) and bool(tree.triggered):
                triggered_tel_ids.append(int(tree.telescope_id))
        root_file.Close()
    except Exception:
        triggered_tel_ids = []
    _ROOT_TRIGGERED_CACHE[cache_key] = tuple(sorted(set(triggered_tel_ids)))
    return np.asarray(_ROOT_TRIGGERED_CACHE[cache_key], dtype=int)


def _event_pointing_azimuth_deg(event, source=None) -> Optional[float]:
    pointing = getattr(event, "pointing", None)
    if pointing is not None:
        for attr in ("array_azimuth", "azimuth"):
            value = getattr(pointing, attr, None)
            if value is not None and np.isfinite(value):
                return float(np.rad2deg(value))
    if source is not None:
        return _root_pointing_azimuth_deg(source)
    return None


def _triggered_tel_ids(event, fallback: Iterable[int] = (), source=None) -> np.ndarray:
    original_triggered = getattr(event, "_pylast_original_triggered_tels", None)
    if original_triggered is not None:
        original_array = np.asarray(list(original_triggered), dtype=int)
        if original_array.size:
            return original_array
    simulation = getattr(event, "simulation", None)
    triggered = getattr(simulation, "triggered_tels", None)
    if triggered is not None:
        triggered_array = np.asarray(list(triggered), dtype=int)
        if triggered_array.size:
            return triggered_array
    if source is not None:
        from_root = _root_triggered_tel_ids(source, _event_id(event))
        if from_root.size:
            return from_root
    r1 = getattr(event, "r1", None)
    r1_tels = getattr(r1, "tels", None) if r1 is not None else None
    if r1_tels:
        return np.asarray(sorted(int(tel_id) for tel_id in r1_tels), dtype=int)
    return np.asarray(list(fallback), dtype=int)


def _selected_image_tel_ids(
    event,
    data: EventData,
    hillas: Mapping[int, HillasParameters],
    image_level: str,
    include_non_triggered: bool,
    only_hillas_tels: bool,
    source=None,
) -> Sequence[int]:
    if only_hillas_tels:
        return sorted(hillas)
    if include_non_triggered:
        return sorted(int(tel_id) for tel_id in data.active_tels)

    selected = _triggered_tel_ids(event, source=source)
    if selected.size == 0 and image_level in {"simulation", "dl1"}:
        # Some sources do not preserve a triggered-telescope list at every
        # stage. Simtel true-image plots use simulation.tels directly, and
        # ImageProcessor fills event.dl1 while rebuilding simulation-level
        # products. In those cases, use the telescopes with nonzero images.
        selected = data.active_tels
    return sorted(int(tel_id) for tel_id in selected)


def _enu_from_az_zd(azimuth_rad: float, zenith_rad: float) -> np.ndarray:
    sin_z, cos_z = np.sin(zenith_rad), np.cos(zenith_rad)
    sin_a, cos_a = np.sin(azimuth_rad), np.cos(azimuth_rad)
    return np.array([sin_z * sin_a, sin_z * cos_a, cos_z])


def _camera_basis(azimuth_rad: float, zenith_rad: float):
    cos_a, sin_a = np.cos(azimuth_rad), np.sin(azimuth_rad)
    cos_z, sin_z = np.cos(zenith_rad), np.sin(zenith_rad)
    optical_axis = np.array([sin_z * sin_a, sin_z * cos_a, cos_z])
    e_az = np.array([cos_a, -sin_a, 0.0])
    e_el = np.array([-cos_z * sin_a, -cos_z * cos_a, sin_z])
    return e_az, e_el, optical_axis


def incident_point_on_camera(
    source_azimuth_rad: float,
    source_zenith_rad: float,
    telescope_azimuth_rad: float,
    telescope_zenith_rad: float,
    camera_rotation_deg: float = 0.0,
    focal_length: Optional[float] = None,
    flip_x: float = 1.0,
    flip_y: float = 1.0,
):
    """Project a sky direction onto the telescope camera plane."""

    e_az, e_el, optical_axis = _camera_basis(telescope_azimuth_rad, telescope_zenith_rad)
    source = _enu_from_az_zd(source_azimuth_rad, source_zenith_rad)

    camera_x = source @ e_az
    camera_y = source @ e_el
    camera_z = source @ optical_axis
    theta_x = np.arctan2(camera_x, camera_z)
    theta_y = np.arctan2(camera_y, camera_z)

    cos_rot = np.cos(_rad(camera_rotation_deg))
    sin_rot = np.sin(_rad(camera_rotation_deg))
    theta_x_rot = cos_rot * theta_x + sin_rot * theta_y
    theta_y_rot = -sin_rot * theta_x + cos_rot * theta_y
    theta_x_rot *= flip_x
    theta_y_rot *= flip_y

    if focal_length is None:
        return theta_x_rot, theta_y_rot, None, None
    return theta_x_rot, theta_y_rot, focal_length * theta_x_rot, focal_length * theta_y_rot


def _extract_subarray_geometry(source) -> Dict[int, TelescopeGeometry]:
    geometries: Dict[int, TelescopeGeometry] = {}
    subarray = source.subarray
    for tel_id, tel_position in subarray.tel_positions.items():
        telescope = subarray.tels[tel_id]
        optics = _first_attr(telescope, "optics", "optics_description")
        camera = _first_attr(telescope, "camera", "camera_description")
        geometry = _first_attr(camera, "geometry", "camera_geometry")

        pix_area = _to_numpy(geometry.pix_area)
        geometries[int(tel_id)] = TelescopeGeometry(
            tel_id=int(tel_id),
            pos_x=float(-tel_position[1]),
            pos_y=float(tel_position[0]),
            focal_length=float(optics.equivalent_focal_length) * 100.0,
            pix_x=_to_numpy(geometry.pix_x) * 100.0,
            pix_y=_to_numpy(geometry.pix_y) * 100.0,
            pix_size=np.sqrt(pix_area) * 100.0,
        )
    return geometries


def _event_id(event) -> int:
    return int(getattr(event, "event_id", getattr(event, "count", 0)))


def _shower(event):
    if not hasattr(event, "simulation") or event.simulation is None:
        raise ValueError("event has no simulation shower information")
    return event.simulation.shower


def _image_from_event(event, tel_id: int, image_level: str) -> np.ndarray:
    if image_level == "simulation":
        if event.simulation is None or tel_id not in event.simulation.tels:
            return np.array([], dtype=float)
        return _to_numpy(event.simulation.tels[tel_id].true_image)

    if image_level == "dl0":
        if not hasattr(event, "dl0") or event.dl0 is None or tel_id not in event.dl0.tels:
            return np.array([], dtype=float)
        return _to_numpy(event.dl0.tels[tel_id].image)

    if image_level == "dl1":
        if not hasattr(event, "dl1") or event.dl1 is None or tel_id not in event.dl1.tels:
            return np.array([], dtype=float)
        camera = event.dl1.tels[tel_id]
        image = _to_numpy(camera.image)
        if hasattr(camera, "mask") and camera.mask is not None:
            image = image * _to_numpy(camera.mask, dtype=bool)
        return image

    raise ValueError("image_level must be one of: simulation, dl0, dl1")


def read_event_data(event, tel_geoms: Mapping[int, TelescopeGeometry], image_level: str = "simulation") -> EventData:
    """Extract event metadata and per-telescope images."""

    shower = _shower(event)
    image_by_tel: Dict[int, np.ndarray] = {}
    image_sum_by_tel: Dict[int, float] = {}

    for tel_id in sorted(tel_geoms):
        image = _image_from_event(event, tel_id, image_level)
        image_by_tel[tel_id] = image
        image_sum_by_tel[tel_id] = float(np.sum(image)) if image.size else 0.0

    active_tels = np.array(
        [tel_id for tel_id, image_sum in image_sum_by_tel.items() if image_sum > 0.0],
        dtype=int,
    )

    altitude = float(shower.alt)
    azimuth = float(shower.az)
    return EventData(
        event_id=_event_id(event),
        energy=float(shower.energy),
        core_x=float(-shower.core_y),
        core_y=float(shower.core_x),
        zenith_deg=float(90.0 - _deg(altitude)),
        azimuth_deg=float(_deg(azimuth)),
        x_max=float(shower.x_max),
        first_interaction_height=float(shower.h_first_int),
        image_by_tel=image_by_tel,
        image_sum_by_tel=image_sum_by_tel,
        active_tels=active_tels,
    )


def event_summary(events: Sequence, tel_geoms: Mapping[int, TelescopeGeometry], image_level: str = "simulation"):
    """Return simple energy and image-size summary statistics."""

    stats = {
        "total_events": len(events),
        "active_events": 0,
        "min_energy": float("inf"),
        "max_energy": -float("inf"),
        "min_image_sum": float("inf"),
        "max_image_sum": -float("inf"),
        "min_energy_event": -1,
        "max_energy_event": -1,
        "min_image_sum_event": -1,
        "max_image_sum_event": -1,
    }

    for index, event in enumerate(events):
        data = read_event_data(event, tel_geoms, image_level=image_level)
        if len(data.active_tels) == 0:
            continue
        stats["active_events"] += 1
        total_image_sum = float(sum(data.image_sum_by_tel.values()))

        if data.energy < stats["min_energy"]:
            stats["min_energy"] = data.energy
            stats["min_energy_event"] = index
        if data.energy > stats["max_energy"]:
            stats["max_energy"] = data.energy
            stats["max_energy_event"] = index
        if total_image_sum < stats["min_image_sum"]:
            stats["min_image_sum"] = total_image_sum
            stats["min_image_sum_event"] = index
        if total_image_sum > stats["max_image_sum"]:
            stats["max_image_sum"] = total_image_sum
            stats["max_image_sum_event"] = index

    return stats


def find_closest_event(
    events: Sequence,
    tel_geoms: Mapping[int, TelescopeGeometry],
    image_level: str = "simulation",
    target_energy: Optional[float] = None,
    target_image_sum: Optional[float] = None,
    target_ntel: Optional[int] = None,
    target_xmax: Optional[float] = None,
) -> Optional[int]:
    """Find the event index closest to requested shower/image properties."""

    targets = [target_energy, target_image_sum, target_ntel, target_xmax]
    if all(target is None for target in targets):
        return None

    best_index = None
    best_distance = float("inf")
    for index, event in enumerate(events):
        data = read_event_data(event, tel_geoms, image_level=image_level)
        if len(data.active_tels) == 0:
            continue

        distance = 0.0
        weights = 0
        if target_energy:
            distance += ((data.energy - target_energy) / target_energy) ** 2
            weights += 1
        if target_image_sum:
            total_image_sum = float(sum(data.image_sum_by_tel.values()))
            distance += ((total_image_sum - target_image_sum) / target_image_sum) ** 2
            weights += 1
        if target_ntel:
            distance += ((len(data.active_tels) - target_ntel) / target_ntel) ** 2
            weights += 1
        if target_xmax:
            distance += ((data.x_max - target_xmax) / target_xmax) ** 2
            weights += 1

        if weights == 0:
            continue
        distance = float(np.sqrt(distance / weights))
        if distance < best_distance:
            best_distance = distance
            best_index = index

    return best_index


class EventVisualizer:
    """High-level LACT/pylast event visualizer."""

    def __init__(
        self,
        source,
        enable_secondary_axes: bool = True,
        outline_pixels: bool = True,
        edge_color=(0, 0, 0, 0.5),
        edge_linewidth: float = 0.2,
    ):
        self.source = source
        self.tel_geoms = _extract_subarray_geometry(source)
        self._verts_cache = {}
        self._extent_cache = {}
        self._transparent_plasma = None
        self.enable_secondary_axes = enable_secondary_axes
        self.outline_pixels = outline_pixels
        self.edge_color = edge_color
        self.edge_linewidth = edge_linewidth

    def summarize_events(self, events: Sequence, image_level: str = "simulation"):
        return event_summary(events, self.tel_geoms, image_level=image_level)

    def find_closest_event(self, events: Sequence, image_level: str = "simulation", **targets):
        return find_closest_event(events, self.tel_geoms, image_level=image_level, **targets)

    def plot_telescopes(
        self,
        event,
        output_path: Optional[str] = None,
        image_level: str = "simulation",
        highlighted_tel_ids: Optional[Iterable[int]] = None,
        core_position: Optional[Sequence[float]] = None,
        include_non_triggered: bool = False,
        show_lhaaso_background: bool = True,
        ed_pos_file: Optional[str] = None,
        md_pos_file: Optional[str] = None,
        show: bool = True,
        finish: bool = True,
    ):
        data = read_event_data(event, self.tel_geoms, image_level=image_level)
        tel_ids = sorted(self.tel_geoms)
        east = np.asarray([self.tel_geoms[t].pos_x for t in tel_ids], dtype=float)
        north = np.asarray([self.tel_geoms[t].pos_y for t in tel_ids], dtype=float)
        raw_values = np.asarray([data.image_sum_by_tel[t] for t in tel_ids], dtype=float)
        if highlighted_tel_ids is None:
            highlighted_tel_ids = _triggered_tel_ids(event, source=self.source)
        highlighted_tel_ids = set(int(t) for t in highlighted_tel_ids)
        triggered_mask = np.asarray([t in highlighted_tel_ids for t in tel_ids], dtype=bool)
        values = raw_values if include_non_triggered else np.where(triggered_mask, raw_values, 0.0)

        plt.rcParams.update({
            "font.family": "DejaVu Sans",
            "font.size": 10,
            "axes.labelsize": 11,
            "axes.titlesize": 13,
            "xtick.labelsize": 9,
            "ytick.labelsize": 9,
            "axes.linewidth": 0.8,
        })
        fig, ax = plt.subplots(figsize=(7.6, 7.0))
        span = max(float(np.ptp(east)), float(np.ptp(north)), 1.0)
        pad = 0.18 * span
        marker_size = 86
        background = {}
        if show_lhaaso_background:
            background = draw_lhaaso_background(
                ax,
                ed_pos_file=ed_pos_file,
                md_pos_file=md_pos_file,
                set_limits=False,
                show_legend=False,
            )

        positive = values[values > 0]
        zero_mask = values <= 0.0
        if np.any(zero_mask):
            ax.scatter(
                east[zero_mask],
                north[zero_mask],
                s=marker_size,
                facecolors="none",
                edgecolors="0.25",
                linewidth=0.9,
                zorder=3,
            )
        if positive.size:
            vmin = float(np.min(positive))
            vmax = float(np.max(positive))
            if np.isclose(vmin, vmax):
                pad_value = max(abs(vmax) * 0.05, 1.0)
                vmin = max(0.0, vmax - pad_value)
                vmax = vmax + pad_value
            norm = Normalize(vmin=vmin, vmax=vmax)
            color_mask = values > 0.0
        else:
            norm = Normalize(vmin=0.0, vmax=1.0)
            color_mask = np.zeros_like(values, dtype=bool)
        scatter = ax.scatter(
            east[color_mask],
            north[color_mask],
            s=marker_size,
            c=values[color_mask],
            cmap="inferno",
            norm=norm,
            edgecolor="0.08",
            linewidth=0.35,
            zorder=3,
        )
        if np.any(triggered_mask):
            ax.scatter(
                east[triggered_mask],
                north[triggered_mask],
                s=marker_size * 1.35,
                facecolors="none",
                edgecolors="#d73027",
                linewidth=1.8,
                zorder=6,
            )
        if positive.size > 1:
            cbar = fig.colorbar(scatter, ax=ax, fraction=0.046, pad=0.04)
            cbar.set_label(_total_quantity_label("pe"), fontsize=11)
            cbar.locator = MaxNLocator(nbins=4)
            cbar.formatter = FormatStrFormatter("%.3g")
            cbar.update_ticks()
            cbar.minorticks_off()
            cbar.ax.tick_params(labelsize=9, direction="out", length=3, width=1)

        for tel_id in tel_ids:
            geom = self.tel_geoms[tel_id]
            ax.annotate(
                f"T{tel_id + 1}",
                xy=(geom.pos_x, geom.pos_y),
                xytext=(3.0, 3.0),
                textcoords="offset points",
                ha="left",
                va="bottom",
                fontsize=7.2,
                color="0.12",
            )

        if core_position is None:
            core_x, core_y = data.core_x, data.core_y
        else:
            core_x, core_y = core_position
        ax.scatter(
            [core_x],
            [core_y],
            marker="*",
            s=230,
            c="#d73027",
            edgecolor="white",
            linewidth=0.8,
            label="Core",
            zorder=6,
        )
        if data.azimuth_deg is not None:
            _add_arrival_arrow(ax, core_x, core_y, data.azimuth_deg, span, mode="incoming")
        ax.set_xlabel("x [m]")
        ax.set_ylabel("y [m]")
        ax.set_title(f"LACT array event_id={data.event_id}")
        limit_east = np.concatenate([east, np.asarray([core_x], dtype=float)])
        limit_north = np.concatenate([north, np.asarray([core_y], dtype=float)])
        if show_lhaaso_background:
            bg_x = []
            bg_y = []
            for key in ("ed", "md"):
                arr = background.get(key)
                if arr is not None and np.asarray(arr).size:
                    bg_x.extend(np.asarray(arr)[:, 0].tolist())
                    bg_y.extend(np.asarray(arr)[:, 1].tolist())
            if bg_x and bg_y:
                limit_east = np.concatenate([limit_east, np.asarray(bg_x, dtype=float)])
                limit_north = np.concatenate([limit_north, np.asarray(bg_y, dtype=float)])
        ax.set_xlim(float(np.min(limit_east)) - pad, float(np.max(limit_east)) + pad)
        ax.set_ylim(float(np.min(limit_north)) - pad, float(np.max(limit_north)) + pad)
        ax.set_aspect("equal", adjustable="box")
        ax.grid(True, alpha=0.25, linewidth=0.55)
        ax.tick_params(direction="in", top=True, right=True)
        x_min, x_max = ax.get_xlim()
        y_min, y_max = ax.get_ylim()
        axis_span = min(x_max - x_min, y_max - y_min)
        compass_length = 0.075 * axis_span
        compass_x = x_min + 0.070 * (x_max - x_min)
        compass_y = y_min + 0.085 * (y_max - y_min)
        _add_array_compass(ax, compass_x, compass_y, compass_length)
        pointing_azimuth_deg = _event_pointing_azimuth_deg(event, source=self.source)
        if pointing_azimuth_deg is not None:
            pointing_x = x_min + 0.075 * (x_max - x_min)
            pointing_y = y_max - 0.105 * (y_max - y_min)
            _add_telescope_direction_inset(ax, pointing_x, pointing_y, compass_length, pointing_azimuth_deg)
        info = _event_info_text(data.event_id, core_x, core_y, data.azimuth_deg)
        if info:
            ax.text(
                0.99,
                0.01,
                info,
                transform=ax.transAxes,
                ha="right",
                va="bottom",
                fontsize=8.4,
                color="0.12",
                bbox=dict(boxstyle="round,pad=0.28", facecolor="white", edgecolor="0.55", alpha=0.94),
            )
        fig.tight_layout()
        if finish:
            self._finish(fig, output_path, show)
        return fig, ax

    def draw_event_sdp_planes(
        self,
        ax,
        event,
        tel_ids: Optional[Iterable[int]] = None,
        core_position: Optional[Sequence[float]] = None,
        color: str = "#2563eb",
        show_labels: bool = True,
    ) -> Dict[int, Tuple[float, float]]:
        """Draw ground intersections of telescope-core shower-detector planes."""

        data = read_event_data(event, self.tel_geoms, image_level="dl0")
        if core_position is None:
            core_x, core_y = data.core_x, data.core_y
        else:
            core_x, core_y = float(core_position[0]), float(core_position[1])
        if tel_ids is None:
            tel_ids = _triggered_tel_ids(event, source=self.source)
        xlim = ax.get_xlim()
        ylim = ax.get_ylim()
        span = float(np.hypot(xlim[1] - xlim[0], ylim[1] - ylim[0]))
        directions: Dict[int, Tuple[float, float]] = {}
        first_line = True
        for tel_id in sorted(int(t) for t in tel_ids):
            geom = self.tel_geoms.get(tel_id)
            if geom is None:
                continue
            dx = core_x - geom.pos_x
            dy = core_y - geom.pos_y
            norm = float(np.hypot(dx, dy))
            if not np.isfinite(norm) or norm <= 0.0:
                continue
            ux, uy = dx / norm, dy / norm
            directions[tel_id] = (ux, uy)
            ax.plot(
                [geom.pos_x - span * ux, geom.pos_x + span * ux],
                [geom.pos_y - span * uy, geom.pos_y + span * uy],
                color=color,
                linestyle="-.",
                linewidth=1.35,
                alpha=0.82,
                zorder=5,
                label="SDP plane" if first_line else None,
            )
            first_line = False
            if show_labels:
                ax.text(
                    geom.pos_x + 0.05 * span * ux,
                    geom.pos_y + 0.05 * span * uy,
                    f"T{tel_id + 1} SDP",
                    color=color,
                    fontsize=7.0,
                    ha="left",
                    va="bottom",
                    zorder=7,
                    bbox=dict(facecolor="white", edgecolor="none", alpha=0.70, pad=1.5),
                )
        ax.set_xlim(xlim)
        ax.set_ylim(ylim)
        return directions

    def plot_event_cores(
        self,
        event,
        output_path: Optional[str] = None,
        image_level: str = "dl0",
        include_non_triggered: bool = False,
        show: bool = True,
    ):
        return self.plot_telescopes(
            event,
            output_path=output_path,
            image_level=image_level,
            include_non_triggered=include_non_triggered,
            show_lhaaso_background=True,
            show=show,
        )

    def plot_event_sdp_planes(
        self,
        event,
        output_path: Optional[str] = None,
        image_level: str = "dl0",
        include_non_triggered: bool = False,
        show: bool = True,
    ):
        fig, ax = self.plot_telescopes(
            event,
            output_path=None,
            image_level=image_level,
            include_non_triggered=include_non_triggered,
            show_lhaaso_background=True,
            show=False,
            finish=False,
        )
        data = read_event_data(event, self.tel_geoms, image_level=image_level)
        self.draw_event_sdp_planes(
            ax,
            event,
            tel_ids=_triggered_tel_ids(event, source=self.source),
            core_position=(data.core_x, data.core_y),
        )
        ax.set_title(f"LACT SDP planes event_id={data.event_id}")
        handles, labels = ax.get_legend_handles_labels()
        if handles:
            ax.legend(handles, labels, loc="upper right", fontsize=8, frameon=True)
        fig.tight_layout()
        self._finish(fig, output_path, show)
        return fig, ax

    def plot_event_sdp_planes_3d(
        self,
        event,
        output_path: Optional[str] = None,
        image_level: str = "dl0",
        include_non_triggered: bool = False,
        z_max: float = 1200.0,
        show_reco: bool = True,
        reconstructor: str = "HillasReconstructor",
        show: bool = True,
    ):
        """Draw a 3D SDP diagnostic using telescope positions and shower axis.

        The truth plane for each telescope is defined by the telescope ground
        position and the true shower axis through the true core. When a valid
        DL2 geometry reconstruction is present, reconstructed SDP planes,
        reconstructed core, and reconstructed axis are overlaid for comparison.
        """

        data = read_event_data(event, self.tel_geoms, image_level=image_level)
        hillas = self._get_hillas_parameters(event)
        tel_ids = _selected_image_tel_ids(
            event,
            data,
            hillas,
            image_level,
            include_non_triggered,
            only_hillas_tels=False,
            source=self.source,
        )

        shower = _shower(event)
        core = np.array([data.core_x, data.core_y, 0.0], dtype=float)
        truth_axis = _enu_from_az_zd(float(shower.az), np.pi / 2 - float(shower.alt))
        if truth_axis[2] < 0:
            truth_axis = -truth_axis
        if abs(truth_axis[2]) < 1.0e-6:
            truth_axis[2] = 1.0e-6
        axis_top = core + truth_axis * (z_max / truth_axis[2])

        fig = plt.figure(figsize=(11, 8))
        ax = fig.add_subplot(111, projection="3d")

        all_x = [core[0], axis_top[0]]
        all_y = [core[1], axis_top[1]]
        colors = plt.cm.tab20(np.linspace(0, 1, max(len(tel_ids), 1)))
        plane_half_width = max(80.0, 0.12 * max(1.0, np.ptp([g.pos_x for g in self.tel_geoms.values()])))
        axis_samples = np.linspace(0.0, z_max / truth_axis[2], 2)
        offset_samples = np.linspace(-plane_half_width, plane_half_width, 2)
        if not tel_ids:
            ax.text2D(
                0.02,
                0.96,
                "No triggered telescopes found for this event.",
                transform=ax.transAxes,
                fontsize=10,
                color="#7f1d1d",
            )

        def draw_plane_family(axis, base_core, line_color, surface_color, line_style, alpha, label_prefix):
            family_any = False
            axis_samples_local = np.linspace(0.0, z_max / axis[2], 2)
            for index, tel_id in enumerate(tel_ids):
                geom = self.tel_geoms.get(int(tel_id))
                if geom is None:
                    continue
                tel_ground = np.array([geom.pos_x, geom.pos_y, 0.0], dtype=float)
                ground_vec = tel_ground - base_core
                ground_norm = float(np.linalg.norm(ground_vec[:2]))
                if ground_norm <= 0 or not np.isfinite(ground_norm):
                    continue
                u = ground_vec / ground_norm
                line_ground = np.vstack(
                    [base_core - u * plane_half_width * 1.6, tel_ground + u * plane_half_width * 1.6]
                )
                color = colors[index] if surface_color is None else surface_color
                ax.plot(
                    line_ground[:, 0],
                    line_ground[:, 1],
                    np.zeros(line_ground.shape[0]),
                    color=line_color if line_color is not None else color,
                    linestyle=line_style,
                    linewidth=1.2,
                    alpha=0.95,
                    label=label_prefix if not family_any else None,
                )
                aa, bb = np.meshgrid(offset_samples, axis_samples_local)
                surf = base_core[None, None, :] + aa[..., None] * u[None, None, :] + bb[..., None] * axis[None, None, :]
                ax.plot_surface(
                    surf[..., 0],
                    surf[..., 1],
                    surf[..., 2],
                    color=color,
                    alpha=alpha,
                    linewidth=0,
                    shade=False,
                )
                all_x.extend([line_ground[0, 0], line_ground[1, 0], surf[..., 0].min(), surf[..., 0].max()])
                all_y.extend([line_ground[0, 1], line_ground[1, 1], surf[..., 1].min(), surf[..., 1].max()])
                family_any = True

        for index, tel_id in enumerate(tel_ids):
            geom = self.tel_geoms.get(int(tel_id))
            if geom is None:
                continue
            tel_ground = np.array([geom.pos_x, geom.pos_y, 0.0], dtype=float)
            color = colors[index]
            ax.scatter([geom.pos_x], [geom.pos_y], [0.0], color=color, s=45, edgecolor="black", linewidth=0.4)
            ax.text(geom.pos_x, geom.pos_y, 0.0, f"T{int(tel_id)}", fontsize=8, color=color)
            all_x.append(geom.pos_x)
            all_y.append(geom.pos_y)

        draw_plane_family(
            truth_axis,
            core,
            line_color=None,
            surface_color=None,
            line_style="-",
            alpha=0.13,
            label_prefix="True SDP planes",
        )

        ax.scatter([core[0]], [core[1]], [0.0], marker="*", s=180, color="#b2182b", label="True core")
        ax.plot(
            [core[0], axis_top[0]],
            [core[1], axis_top[1]],
            [core[2], axis_top[2]],
            color="#b2182b",
            linewidth=2.4,
            label="True shower axis / SDP intersection",
        )

        if show_reco:
            geometry = getattr(getattr(event, "dl2", None), "geometry", {}) or {}
            reco = geometry.get(reconstructor)
            if reco is not None and bool(getattr(reco, "is_valid", False)):
                reco_alt = float(getattr(reco, "alt", np.nan))
                reco_az = float(getattr(reco, "az", np.nan))
                if np.isfinite(reco_alt) and np.isfinite(reco_az):
                    reco_core = np.array(
                        [
                            float(getattr(reco, "core_x", core[0])),
                            float(getattr(reco, "core_y", core[1])),
                            0.0,
                        ],
                        dtype=float,
                    )
                    reco_axis = _enu_from_az_zd(reco_az, np.pi / 2 - reco_alt)
                    if reco_axis[2] < 0:
                        reco_axis = -reco_axis
                    if abs(reco_axis[2]) >= 1.0e-6:
                        reco_top = reco_core + reco_axis * (z_max / reco_axis[2])
                        ax.scatter(
                            [reco_core[0]],
                            [reco_core[1]],
                            [0.0],
                            marker="x",
                            s=90,
                            color="#2166ac",
                            label="Reco core",
                        )
                        ax.plot(
                            [reco_core[0], reco_top[0]],
                            [reco_core[1], reco_top[1]],
                            [reco_core[2], reco_top[2]],
                            color="#2166ac",
                            linewidth=2.0,
                            linestyle="--",
                            label=f"{reconstructor} axis",
                        )
                        draw_plane_family(
                            reco_axis,
                            reco_core,
                            line_color="#2166ac",
                            surface_color="#2166ac",
                            line_style="--",
                            alpha=0.075,
                            label_prefix=f"{reconstructor} SDP planes",
                        )
                        all_x.extend([reco_core[0], reco_top[0]])
                        all_y.extend([reco_core[1], reco_top[1]])

        ax.set_xlabel("East (m)")
        ax.set_ylabel("North (m)")
        ax.set_zlabel("Height (m)")
        ax.set_title(f"3D SDP planes event_id={data.event_id}")
        ax.set_zlim(0.0, z_max)
        x_mid = 0.5 * (float(np.min(all_x)) + float(np.max(all_x)))
        y_mid = 0.5 * (float(np.min(all_y)) + float(np.max(all_y)))
        xy_span = max(float(np.ptp(all_x)), float(np.ptp(all_y)), 1.0)
        pad = 0.08 * xy_span
        ax.set_xlim(x_mid - xy_span / 2 - pad, x_mid + xy_span / 2 + pad)
        ax.set_ylim(y_mid - xy_span / 2 - pad, y_mid + xy_span / 2 + pad)
        ax.view_init(elev=24, azim=-58)
        ax.legend(loc="upper left", fontsize=8)
        fig.tight_layout()
        self._finish(fig, output_path, show)
        return fig, ax

    def plot_event_sdp_planes_3d_interactive(
        self,
        event,
        output_html: Optional[str] = None,
        image_level: str = "dl0",
        include_non_triggered: bool = False,
        z_max: float = 1200.0,
        show_reco: bool = True,
        reconstructor: str = "HillasReconstructor",
    ):
        """Draw an interactive 3D SDP diagnostic with Plotly.

        This is intended for notebooks and HTML reports. It uses the same event
        information as :meth:`plot_event_sdp_planes_3d`, but keeps each axis,
        telescope, ground intersection, and SDP plane as a separately hoverable
        object so the event geometry can be inspected by rotating the view.
        """

        try:
            import plotly.graph_objects as go
        except ImportError as exc:
            raise ImportError("plot_event_sdp_planes_3d_interactive requires plotly") from exc

        data = read_event_data(event, self.tel_geoms, image_level=image_level)
        hillas = self._get_hillas_parameters(event)
        tel_ids = _selected_image_tel_ids(
            event,
            data,
            hillas,
            image_level,
            include_non_triggered,
            only_hillas_tels=False,
            source=self.source,
        )

        shower = _shower(event)
        true_core = np.array([data.core_x, data.core_y, 0.0], dtype=float)
        true_axis = _enu_from_az_zd(float(shower.az), np.pi / 2 - float(shower.alt))
        if true_axis[2] < 0:
            true_axis = -true_axis
        if abs(true_axis[2]) < 1.0e-6:
            true_axis[2] = 1.0e-6
        true_top = true_core + true_axis * (z_max / true_axis[2])

        all_x = [true_core[0], true_top[0]]
        all_y = [true_core[1], true_top[1]]
        traces = []
        palette = [mcolors.to_hex(c) for c in plt.cm.tab20(np.linspace(0, 1, max(len(tel_ids), 1)))]
        plane_half_width = max(100.0, 0.16 * max(1.0, np.ptp([g.pos_x for g in self.tel_geoms.values()])))
        axis_samples = np.linspace(0.0, 1.0, 2)
        offset_samples = np.linspace(-plane_half_width, plane_half_width, 2)

        all_tel_x = [geom.pos_x for geom in self.tel_geoms.values()]
        all_tel_y = [geom.pos_y for geom in self.tel_geoms.values()]
        all_tel_names = [f"T{tel_id}" for tel_id in self.tel_geoms]
        all_x.extend(all_tel_x)
        all_y.extend(all_tel_y)
        traces.append(
            go.Scatter3d(
                x=all_tel_x,
                y=all_tel_y,
                z=[0.0] * len(all_tel_x),
                mode="markers",
                name="All telescopes",
                text=all_tel_names,
                hovertemplate="%{text}<br>East=%{x:.1f} m<br>North=%{y:.1f} m<extra></extra>",
                marker=dict(size=3, color="rgba(120,120,120,0.45)"),
                showlegend=True,
            )
        )

        active_x, active_y, active_text, active_color = [], [], [], []
        for index, tel_id in enumerate(tel_ids):
            geom = self.tel_geoms.get(int(tel_id))
            if geom is None:
                continue
            active_x.append(geom.pos_x)
            active_y.append(geom.pos_y)
            active_text.append(f"T{int(tel_id)}")
            active_color.append(palette[index % len(palette)])
            all_x.append(geom.pos_x)
            all_y.append(geom.pos_y)
        if active_x:
            traces.append(
                go.Scatter3d(
                    x=active_x,
                    y=active_y,
                    z=[0.0] * len(active_x),
                    mode="markers+text",
                    name="Triggered telescopes",
                    text=active_text,
                    textposition="top center",
                    hovertemplate="%{text}<br>East=%{x:.1f} m<br>North=%{y:.1f} m<extra></extra>",
                    marker=dict(size=7, color=active_color, line=dict(color="black", width=1.2)),
                )
            )

        def add_axis(core, top, color, name, dash="solid"):
            traces.append(
                go.Scatter3d(
                    x=[core[0], top[0]],
                    y=[core[1], top[1]],
                    z=[core[2], top[2]],
                    mode="lines",
                    name=name,
                    line=dict(color=color, width=8, dash=dash),
                    hovertemplate=f"{name}<br>East=%{{x:.1f}} m<br>North=%{{y:.1f}} m<br>Height=%{{z:.1f}} m<extra></extra>",
                )
            )

        def add_core(core, color, symbol, name, size):
            traces.append(
                go.Scatter3d(
                    x=[core[0]],
                    y=[core[1]],
                    z=[0.0],
                    mode="markers",
                    name=name,
                    marker=dict(size=size, color=color, symbol=symbol, line=dict(color="black", width=1.0)),
                    hovertemplate=f"{name}<br>East=%{{x:.1f}} m<br>North=%{{y:.1f}} m<extra></extra>",
                )
            )

        def add_plane_family(axis, core, colors, surface_opacity, line_dash, legend_prefix, group_prefix):
            family_added = False
            scale = z_max / axis[2]
            for index, tel_id in enumerate(tel_ids):
                geom = self.tel_geoms.get(int(tel_id))
                if geom is None:
                    continue
                tel_ground = np.array([geom.pos_x, geom.pos_y, 0.0], dtype=float)
                ground_vec = tel_ground - core
                ground_norm = float(np.linalg.norm(ground_vec[:2]))
                if ground_norm <= 0 or not np.isfinite(ground_norm):
                    continue
                u = ground_vec / ground_norm
                color = colors[index % len(colors)] if isinstance(colors, list) else colors
                aa, bb = np.meshgrid(offset_samples, axis_samples * scale)
                surf = core[None, None, :] + aa[..., None] * u[None, None, :] + bb[..., None] * axis[None, None, :]
                traces.append(
                    go.Surface(
                        x=surf[..., 0].tolist(),
                        y=surf[..., 1].tolist(),
                        z=surf[..., 2].tolist(),
                        name=f"{legend_prefix} T{int(tel_id)}",
                        legendgroup=group_prefix,
                        showscale=False,
                        showlegend=not family_added,
                        opacity=surface_opacity,
                        colorscale=[[0.0, color], [1.0, color]],
                        hovertemplate=(
                            f"{legend_prefix} T{int(tel_id)}<br>"
                            "East=%{x:.1f} m<br>North=%{y:.1f} m<br>Height=%{z:.1f} m<extra></extra>"
                        ),
                    )
                )
                line_a = core - u * plane_half_width * 1.8
                line_b = tel_ground + u * plane_half_width * 1.8
                traces.append(
                    go.Scatter3d(
                        x=[line_a[0], line_b[0]],
                        y=[line_a[1], line_b[1]],
                        z=[0.0, 0.0],
                        mode="lines",
                        name=f"{legend_prefix} ground lines",
                        legendgroup=group_prefix,
                        showlegend=False,
                        line=dict(color=color, width=4, dash=line_dash),
                        hovertemplate=f"{legend_prefix} T{int(tel_id)} ground line<extra></extra>",
                    )
                )
                all_x.extend([line_a[0], line_b[0], float(np.min(surf[..., 0])), float(np.max(surf[..., 0]))])
                all_y.extend([line_a[1], line_b[1], float(np.min(surf[..., 1])), float(np.max(surf[..., 1]))])
                family_added = True

        if tel_ids:
            add_plane_family(
                true_axis,
                true_core,
                palette,
                surface_opacity=0.18,
                line_dash="solid",
                legend_prefix="Truth SDP",
                group_prefix="truth_sdp",
            )
        add_core(true_core, "#b2182b", "diamond", "True core", 8)
        add_axis(true_core, true_top, "#b2182b", "True shower axis", dash="solid")

        if show_reco:
            geometry = getattr(getattr(event, "dl2", None), "geometry", {}) or {}
            reco = geometry.get(reconstructor)
            if reco is not None and bool(getattr(reco, "is_valid", False)):
                reco_alt = float(getattr(reco, "alt", np.nan))
                reco_az = float(getattr(reco, "az", np.nan))
                if np.isfinite(reco_alt) and np.isfinite(reco_az):
                    reco_core = np.array(
                        [
                            float(getattr(reco, "core_x", true_core[0])),
                            float(getattr(reco, "core_y", true_core[1])),
                            0.0,
                        ],
                        dtype=float,
                    )
                    reco_axis = _enu_from_az_zd(reco_az, np.pi / 2 - reco_alt)
                    if reco_axis[2] < 0:
                        reco_axis = -reco_axis
                    if abs(reco_axis[2]) >= 1.0e-6:
                        reco_top = reco_core + reco_axis * (z_max / reco_axis[2])
                        add_plane_family(
                            reco_axis,
                            reco_core,
                            "#2166ac",
                            surface_opacity=0.10,
                            line_dash="dash",
                            legend_prefix=f"{reconstructor} SDP",
                            group_prefix="reco_sdp",
                        )
                        add_core(reco_core, "#2166ac", "x", "Reco core", 8)
                        add_axis(reco_core, reco_top, "#2166ac", f"{reconstructor} axis", dash="dash")
                        all_x.extend([reco_core[0], reco_top[0]])
                        all_y.extend([reco_core[1], reco_top[1]])

        xy_span = max(float(np.ptp(all_x)), float(np.ptp(all_y)), 1.0)
        x_mid = 0.5 * (float(np.min(all_x)) + float(np.max(all_x)))
        y_mid = 0.5 * (float(np.min(all_y)) + float(np.max(all_y)))
        pad = 0.10 * xy_span
        fig = go.Figure(data=traces)
        fig.update_layout(
            title=f"3D SDP geometry event_id={data.event_id}",
            template="plotly_white",
            width=1000,
            height=760,
            legend=dict(x=0.02, y=0.98, bgcolor="rgba(255,255,255,0.72)"),
            margin=dict(l=0, r=0, t=50, b=0),
            scene=dict(
                xaxis=dict(title="East (m)", range=[x_mid - xy_span / 2 - pad, x_mid + xy_span / 2 + pad]),
                yaxis=dict(title="North (m)", range=[y_mid - xy_span / 2 - pad, y_mid + xy_span / 2 + pad]),
                zaxis=dict(title="Height (m)", range=[0.0, z_max]),
                aspectmode="data",
                camera=dict(eye=dict(x=1.55, y=-1.85, z=1.15), up=dict(x=0, y=0, z=1)),
            ),
            annotations=[
                dict(
                    text="No triggered telescopes found; only array layout and shower axis are shown.",
                    showarrow=False,
                    x=0.02,
                    y=0.02,
                    xref="paper",
                    yref="paper",
                    font=dict(color="#7f1d1d", size=12),
                    bgcolor="rgba(255,255,255,0.82)",
                    bordercolor="#fecaca",
                )
            ]
            if not tel_ids
            else [],
        )
        if output_html:
            fig.write_html(output_html, include_plotlyjs="cdn", full_html=True)
        return fig

    def plot_event(
        self,
        event,
        output_path: Optional[str] = None,
        image_level: str = "simulation",
        show_hillas: bool = False,
        only_hillas_tels: bool = False,
        include_non_triggered: bool = False,
        show_ideal_position: bool = False,
        show: bool = True,
    ):
        data = read_event_data(event, self.tel_geoms, image_level=image_level)
        hillas = self._get_hillas_parameters(event)

        tel_ids = _selected_image_tel_ids(
            event,
            data,
            hillas,
            image_level,
            include_non_triggered,
            only_hillas_tels,
            source=self.source,
        )
        if not tel_ids:
            return None, None

        cols = max(1, int(np.sqrt(len(tel_ids) + 1)))
        rows = int(np.ceil((len(tel_ids) + 1) / cols))
        fig, axes = plt.subplots(rows, cols, figsize=(6 * cols + 2, 6 * rows))
        axes = np.asarray(axes).reshape(-1)

        axes[0].axis("off")
        axes[0].text(
            0.05,
            0.5,
            self._event_text(data, len(tel_ids)),
            fontsize=22,
            fontweight="bold",
            ha="left",
            va="center",
            transform=axes[0].transAxes,
            linespacing=1.3,
        )

        for index, tel_id in enumerate(tel_ids, start=1):
            image = data.image_by_tel[tel_id]
            norm, cmap = self._image_norm(image)
            self._draw_camera_image(axes[index], self.tel_geoms[tel_id], image, norm, cmap)
            axes[index].text(
                0.05,
                0.96,
                f"Telescope {tel_id + 1}",
                transform=axes[index].transAxes,
                fontsize=11,
                color="black",
                va="top",
                ha="left",
                bbox=dict(facecolor="white", alpha=0.7, edgecolor="none"),
            )
            if show_hillas and tel_id in hillas:
                self._draw_hillas_ellipse(axes[index], hillas[tel_id])
            if show_ideal_position:
                self._draw_ideal_position(axes[index], event)

        for ax in axes[len(tel_ids) + 1 :]:
            ax.axis("off")

        fig.tight_layout(pad=1.0)
        self._finish(fig, output_path, show)
        return fig, axes

    def plot_cleaned_event(self, event, output_path: Optional[str] = None, show_hillas: bool = True, show: bool = True):
        return self.plot_event(
            event,
            output_path=output_path,
            image_level="dl1",
            show_hillas=show_hillas,
            only_hillas_tels=False,
            show=show,
        )

    def plot_gathered_event(
        self,
        event,
        output_path: Optional[str] = None,
        image_level: str = "simulation",
        show_hillas: bool = True,
        only_hillas: bool = False,
        only_image: bool = False,
        only_hillas_tels: bool = False,
        include_non_triggered: bool = False,
        zero_eps: float = 0.0,
        show_colorbar: bool = False,
        show_ideal_position: bool = False,
        show: bool = True,
    ):
        if only_hillas and only_image:
            raise ValueError("only_hillas and only_image cannot both be true")

        data = read_event_data(event, self.tel_geoms, image_level=image_level)
        hillas = self._get_hillas_parameters(event)
        tel_ids = _selected_image_tel_ids(
            event,
            data,
            hillas,
            image_level,
            include_non_triggered,
            only_hillas_tels,
            source=self.source,
        )
        if not tel_ids:
            return None, None

        fig, axes = plt.subplots(1, 2, figsize=(14, 6))
        info_ax, ax = axes
        info_ax.axis("off")
        info_ax.text(
            0.05,
            0.5,
            self._event_text(data, len(tel_ids)),
            fontsize=22,
            fontweight="bold",
            ha="left",
            va="center",
            transform=info_ax.transAxes,
            linespacing=1.3,
        )

        for tel_id in tel_ids:
            self._draw_camera_frame(ax, self.tel_geoms[tel_id], edgecolor=(0, 0, 0, 0.35), lw=0.25, zorder=1)

        cmap = self._transparent_zero_cmap()
        caxes = []
        if show_colorbar:
            divider = make_axes_locatable(ax)
            for _ in tel_ids:
                caxes.append(divider.append_axes("right", size="2%", pad=0.3))

        for index, tel_id in enumerate(tel_ids):
            geom = self.tel_geoms[tel_id]
            if not only_hillas:
                image = np.asarray(data.image_by_tel[tel_id], dtype=float)
                if zero_eps > 0:
                    image = image.copy()
                    image[image <= zero_eps] = 0.0
                max_value = max(float(np.max(image)) if np.any(image > 0) else 1.0, 1.0)
                bounds = [0, 1] + np.linspace(1, max_value, 256).tolist()
                norm = mcolors.BoundaryNorm(bounds, cmap.N)
                verts = self._vertices_for(geom)
                pc = PolyCollection(
                    verts,
                    array=image,
                    cmap=cmap,
                    norm=norm,
                    edgecolors="none",
                    linewidth=0.0,
                    rasterized=True,
                    zorder=2 + index * 0.01,
                )
                ax.add_collection(pc)
                ax.text(
                    0.02,
                    0.98 - 0.04 * index,
                    f"Tel {tel_id + 1} (max {max_value:.1f})",
                    transform=ax.transAxes,
                    fontsize=10,
                    color="black",
                    va="top",
                    ha="left",
                    bbox=dict(facecolor="white", alpha=0.6, edgecolor="none"),
                )
                if show_colorbar:
                    sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
                    sm.set_array([])
                    plt.colorbar(sm, cax=caxes[index], label=f"Tel {tel_id + 1}", orientation="vertical")

            if show_hillas and not only_image and tel_id in hillas:
                self._draw_hillas_ellipse(ax, hillas[tel_id])

        if show_ideal_position:
            self._draw_ideal_position(ax, event)

        fig.tight_layout(pad=1.0)
        self._finish(fig, output_path, show)
        return fig, axes

    def plot_custom_event(
        self,
        event,
        images: Mapping[int, np.ndarray],
        masks: Optional[Mapping[int, np.ndarray]] = None,
        hillas: Optional[Mapping[int, object]] = None,
        incident_positions: Optional[Mapping[int, Mapping[str, float]]] = None,
        output_path: Optional[str] = None,
        show: bool = True,
    ):
        data = read_event_data(event, self.tel_geoms, image_level="simulation")
        tel_ids = sorted(images.keys())
        if masks is not None:
            tel_ids = [tel_id for tel_id in tel_ids if np.max(masks[tel_id]) > 0]
        if hillas is not None:
            hillas_params = self._hillas_from_mapping(hillas)
            tel_ids = sorted(hillas_params)
        else:
            hillas_params = {}

        if not tel_ids:
            return None, None

        cols = max(1, int(np.sqrt(len(tel_ids) + 1)))
        rows = int(np.ceil((len(tel_ids) + 1) / cols))
        fig, axes = plt.subplots(rows, cols, figsize=(6 * cols + 2, 6 * rows))
        axes = np.asarray(axes).reshape(-1)
        axes[0].axis("off")
        axes[0].text(
            0.05,
            0.5,
            self._event_text(data, len(tel_ids)),
            fontsize=22,
            fontweight="bold",
            ha="left",
            va="center",
            transform=axes[0].transAxes,
            linespacing=1.3,
        )

        for index, tel_id in enumerate(tel_ids, start=1):
            image = np.asarray(images[tel_id], dtype=float)
            if masks is not None:
                image = image * np.asarray(masks[tel_id], dtype=bool)
            if image.size == 0 or np.max(image) <= 0:
                continue
            norm, cmap = self._image_norm(image)
            self._draw_camera_image(axes[index], self.tel_geoms[tel_id], image, norm, cmap)
            axes[index].text(
                0.05,
                0.96,
                f"Telescope {tel_id + 1}",
                transform=axes[index].transAxes,
                fontsize=11,
                va="top",
                ha="left",
                bbox=dict(facecolor="white", alpha=0.7, edgecolor="none"),
            )
            if tel_id in hillas_params:
                self._draw_hillas_ellipse(axes[index], hillas_params[tel_id])
            if incident_positions and tel_id in incident_positions:
                pos = incident_positions[tel_id]
                axes[index].plot(pos["x_hat"], pos["y_hat"], marker="x", linestyle="None", color="cyan", ms=5, zorder=10000)

        for ax in axes[len(tel_ids) + 1 :]:
            ax.axis("off")

        fig.tight_layout(pad=1.0)
        self._finish(fig, output_path, show)
        return fig, axes

    def _event_text(self, data: EventData, n_tel: int) -> str:
        return (
            f"Energy: {data.energy:.2f} TeV\n"
            f"Core: (x={data.core_x:.1f}, y={data.core_y:.1f}) m\n"
            f"Direction: (ze={data.zenith_deg:.1f} deg, az={data.azimuth_deg:.1f} deg)\n"
            f"Xmax: {data.x_max:.1f} g/cm2\n"
            f"First Int: {data.first_interaction_height:.1f} m\n"
            f"Active Tels: {n_tel}"
        )

    def _image_norm(self, image: np.ndarray):
        max_value = max(float(np.max(image)) if image.size else 1.0, 1.0)
        colors = [(1, 1, 1, 1)] + plt.cm.plasma(np.linspace(0, 1, 256)).tolist()
        cmap = mcolors.ListedColormap(colors)
        bounds = [0, 1] + np.linspace(1, max_value, 256).tolist()
        return mcolors.BoundaryNorm(bounds, cmap.N), cmap

    def _vertices_for(self, tel_geom: TelescopeGeometry) -> np.ndarray:
        key = tel_geom.tel_id
        if key not in self._verts_cache:
            x = tel_geom.pix_x.astype(float)
            y = tel_geom.pix_y.astype(float)
            size = tel_geom.pix_size.astype(float)
            x0 = (x - size / 2)[:, None]
            x1 = (x + size / 2)[:, None]
            y0 = (y - size / 2)[:, None]
            y1 = (y + size / 2)[:, None]
            verts = np.concatenate(
                [
                    np.stack([x0, y0], axis=2),
                    np.stack([x0, y1], axis=2),
                    np.stack([x1, y1], axis=2),
                    np.stack([x1, y0], axis=2),
                    np.stack([x0, y0], axis=2),
                ],
                axis=1,
            ).astype(float)
            self._verts_cache[key] = verts
            pad = float(np.max(size))
            self._extent_cache[key] = (
                (float(x.min() - pad), float(x.max() + pad)),
                (float(y.min() - pad), float(y.max() + pad)),
            )
        return self._verts_cache[key]

    def _draw_camera_image(self, ax, tel_geom: TelescopeGeometry, image: np.ndarray, norm, cmap, add_colorbar: bool = True):
        verts = self._vertices_for(tel_geom)
        pc = PolyCollection(
            verts,
            array=np.asarray(image, dtype=float),
            cmap=cmap,
            norm=norm,
            edgecolor=self.edge_color if self.outline_pixels else "none",
            linewidth=self.edge_linewidth if self.outline_pixels else 0.0,
            rasterized=True,
        )
        ax.add_collection(pc)
        self._format_camera_axes(ax, tel_geom)
        if add_colorbar:
            divider = make_axes_locatable(ax)
            cax = divider.append_axes("right", size="5%", pad=0.6)
            sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
            sm.set_array([])
            plt.colorbar(sm, cax=cax, label="PE")

    def _draw_camera_frame(self, ax, tel_geom: TelescopeGeometry, edgecolor=(0, 0, 0, 0.35), lw: float = 0.25, zorder: int = 1):
        verts = self._vertices_for(tel_geom)
        pc = PolyCollection(verts, facecolors="none", edgecolors=edgecolor, linewidth=lw, rasterized=False, zorder=zorder)
        ax.add_collection(pc)
        self._format_camera_axes(ax, tel_geom)

    def _format_camera_axes(self, ax, tel_geom: TelescopeGeometry):
        xlim, ylim = self._extent_cache[tel_geom.tel_id]
        ax.set_xlim(*xlim)
        ax.set_ylim(*ylim)
        ax.set_aspect("equal")
        ax.set_xlabel("X Position (cm)")
        ax.set_ylabel("Y Position (cm)")
        if self.enable_secondary_axes and not hasattr(ax, "_pylast_secondary_axes_added"):
            secax_x = ax.secondary_xaxis(
                "top",
                functions=(
                    lambda x: np.degrees(np.arctan(x / tel_geom.focal_length)),
                    lambda deg: np.tan(np.radians(deg)) * tel_geom.focal_length,
                ),
            )
            secax_x.set_xlabel("X (degrees)")
            secax_y = ax.secondary_yaxis(
                "right",
                functions=(
                    lambda y: np.degrees(np.arctan(y / tel_geom.focal_length)),
                    lambda deg: np.tan(np.radians(deg)) * tel_geom.focal_length,
                ),
            )
            secax_y.set_ylabel("Y (degrees)")
            ax._pylast_secondary_axes_added = True

    def _transparent_zero_cmap(self):
        if self._transparent_plasma is None:
            base = plt.cm.plasma(np.linspace(0, 1, 256))
            self._transparent_plasma = mcolors.ListedColormap([(0, 0, 0, 0.0)] + [tuple(rgba) for rgba in base])
        return self._transparent_plasma

    def _get_hillas_parameters(self, event) -> Dict[int, HillasParameters]:
        if not hasattr(event, "dl1") or event.dl1 is None or not hasattr(event.dl1, "tels"):
            return {}
        hillas = {}
        for tel_id, dl1 in event.dl1.tels.items():
            if tel_id not in self.tel_geoms:
                continue
            if not hasattr(dl1, "image_parameters") or not hasattr(dl1.image_parameters, "hillas"):
                continue
            params = dl1.image_parameters.hillas
            hillas[tel_id] = self._scale_hillas(tel_id, params)
        return hillas

    def _hillas_from_mapping(self, hillas_mapping: Mapping[int, object]) -> Dict[int, HillasParameters]:
        hillas = {}
        for tel_id, params in hillas_mapping.items():
            if params is None or tel_id not in self.tel_geoms:
                continue
            hillas[tel_id] = self._scale_hillas(tel_id, params)
        return hillas

    def _scale_hillas(self, tel_id: int, params) -> HillasParameters:
        geom = self.tel_geoms[tel_id]
        cog_x = getattr(params, "x", getattr(params, "cog_x", 0.0))
        cog_y = getattr(params, "y", getattr(params, "cog_y", 0.0))
        return HillasParameters(
            length=float(params.length) * geom.focal_length * 2,
            width=float(params.width) * geom.focal_length * 2,
            psi=float(params.psi) * 180.0 / np.pi,
            cog_x=float(cog_x) * geom.focal_length,
            cog_y=float(cog_y) * geom.focal_length,
        )

    def _draw_hillas_ellipse(self, ax, hillas: HillasParameters):
        ellipse = Ellipse(
            xy=(hillas.cog_x, hillas.cog_y),
            width=hillas.length,
            height=hillas.width,
            angle=hillas.psi,
            edgecolor="r",
            facecolor="none",
            lw=2,
            zorder=10000,
        )
        ax.add_patch(ellipse)
        ax.plot(hillas.cog_x, hillas.cog_y, marker="o", linestyle="None", color="r", ms=3, zorder=10000)

    def _draw_ideal_position(self, ax, event):
        if not hasattr(event, "pointing") or event.pointing is None:
            return
        shower = _shower(event)
        first_tel = next(iter(self.tel_geoms))
        focal_length = self.tel_geoms[first_tel].focal_length
        _, _, x_camera, y_camera = incident_point_on_camera(
            source_azimuth_rad=float(shower.az),
            source_zenith_rad=np.pi / 2 - float(shower.alt),
            telescope_azimuth_rad=float(event.pointing.array_azimuth),
            telescope_zenith_rad=np.pi / 2 - float(event.pointing.array_altitude),
            focal_length=focal_length,
        )
        if x_camera is not None and y_camera is not None:
            ax.plot(y_camera, x_camera, marker="x", linestyle="None", color="magenta", ms=6, zorder=10000)

    def _finish(self, fig, output_path: Optional[str], show: bool):
        if output_path:
            fig.savefig(output_path, dpi=300, bbox_inches="tight")
        if show:
            plt.show()
        else:
            plt.close(fig)


Visualizer = EventVisualizer


__all__ = [
    "EventData",
    "EventVisualizer",
    "HillasParameters",
    "TelescopeGeometry",
    "Visualizer",
    "event_summary",
    "find_closest_event",
    "incident_point_on_camera",
    "read_event_data",
]
