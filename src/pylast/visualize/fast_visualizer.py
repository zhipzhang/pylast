"""
Fast event visualization helpers for pylast.

This module is based on the LACT event plotting workflow, but is written as a
package module: no local paths, no notebook-only state, and no hard dependency
on a specific input file.  It focuses on quick camera drawing with
PolyCollection and cached pixel vertices.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, Iterable, Mapping, Optional, Sequence

import matplotlib
import matplotlib.colors as mcolors
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.collections import PolyCollection
from matplotlib.patches import Ellipse
from mpl_toolkits.axes_grid1 import make_axes_locatable


matplotlib.rcParams["path.simplify"] = True
matplotlib.rcParams["agg.path.chunksize"] = 20000


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
            pos_x=float(tel_position[0]),
            pos_y=float(tel_position[1]),
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

    for tel_id in tel_geoms:
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
        core_x=float(shower.core_x),
        core_y=float(shower.core_y),
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
        show: bool = True,
    ):
        data = read_event_data(event, self.tel_geoms, image_level=image_level)
        tel_ids = list(self.tel_geoms)
        positions = np.array([(self.tel_geoms[t].pos_x, self.tel_geoms[t].pos_y) for t in tel_ids])
        image_sums = np.array([data.image_sum_by_tel[t] for t in tel_ids])
        colors = np.log10(np.clip(image_sums, 1.0, None))

        if highlighted_tel_ids is None:
            highlighted_tel_ids = data.active_tels
        highlighted_tel_ids = set(int(t) for t in highlighted_tel_ids)
        highlighted = np.array([t in highlighted_tel_ids for t in tel_ids], dtype=bool)

        fig, ax = plt.subplots(figsize=(10, 8))
        scatter = ax.scatter(
            positions[:, 0],
            positions[:, 1],
            c=colors,
            cmap="viridis",
            s=300,
            edgecolor="k",
            alpha=0.8,
        )
        ax.scatter(
            positions[highlighted, 0],
            positions[highlighted, 1],
            facecolors="none",
            edgecolors="r",
            s=350,
            linewidths=2,
        )

        for tel_id in tel_ids:
            geom = self.tel_geoms[tel_id]
            ax.text(geom.pos_x, geom.pos_y + 30, str(tel_id), fontsize=12, ha="center", va="center")

        if core_position is None:
            core_x, core_y = data.core_x, data.core_y
        else:
            core_x, core_y = core_position
        ax.scatter(core_x, core_y, color="red", marker="*", s=300, label="Shower Core")
        direction_x = -np.sin(np.radians(data.azimuth_deg))
        direction_y = np.cos(np.radians(data.azimuth_deg))
        ax.plot(
            [core_x, core_x + direction_x * 800],
            [core_y, core_y + direction_y * 800],
            color="red",
            linestyle="--",
            label="Shower Direction",
        )

        ax.set_xlabel("X Position (m)")
        ax.set_ylabel("Y Position (m)")
        ax.set_title("Telescope Positions and Shower Core")
        ax.set_xlim(-850, 850)
        ax.set_ylim(-850, 850)
        ax.legend()
        fig.colorbar(scatter, ax=ax, label="log10(image sum)")
        self._finish(fig, output_path, show)
        return fig, ax

    def plot_event(
        self,
        event,
        output_path: Optional[str] = None,
        image_level: str = "simulation",
        show_hillas: bool = False,
        only_hillas_tels: bool = False,
        show_ideal_position: bool = False,
        show: bool = True,
    ):
        data = read_event_data(event, self.tel_geoms, image_level=image_level)
        hillas = self._get_hillas_parameters(event)

        tel_ids = list(data.active_tels)
        if only_hillas_tels:
            tel_ids = list(hillas)
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
                f"Telescope {tel_id}",
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
        zero_eps: float = 0.0,
        show_colorbar: bool = False,
        show_ideal_position: bool = False,
        show: bool = True,
    ):
        if only_hillas and only_image:
            raise ValueError("only_hillas and only_image cannot both be true")

        data = read_event_data(event, self.tel_geoms, image_level=image_level)
        hillas = self._get_hillas_parameters(event)
        tel_ids = list(hillas) if only_hillas_tels else list(data.active_tels)
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
                    f"Tel {tel_id} (max {max_value:.1f})",
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
                    plt.colorbar(sm, cax=caxes[index], label=f"Tel {tel_id}", orientation="vertical")

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
        tel_ids = list(images.keys())
        if masks is not None:
            tel_ids = [tel_id for tel_id in tel_ids if np.max(masks[tel_id]) > 0]
        if hillas is not None:
            hillas_params = self._hillas_from_mapping(hillas)
            tel_ids = list(hillas_params)
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
                f"Telescope {tel_id}",
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
            f"Core: ({data.core_x:.1f}, {data.core_y:.1f}) m\n"
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
