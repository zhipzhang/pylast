from __future__ import annotations

from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple, Union

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Polygon


_DATA_DIR = Path(__file__).resolve().parent / "data"
DEFAULT_KM2A_ED_POS_FILE = str(_DATA_DIR / "ED_pos_5216up_20220705.txt")
DEFAULT_KM2A_MD_POS_FILE = str(_DATA_DIR / "MD_pos_1188.txt")
WCDA_CENTER_XYZ = (-7.00, 7.97, 0.0)
WCDA_ALPHA_RAD = np.radians(30.0)
_DETECTOR_POS_CACHE: Dict[str, np.ndarray] = {}


def km2a_xy_to_eventshow_map(x_km2a: np.ndarray | float, y_km2a: np.ndarray | float):
    x = np.asarray(x_km2a, dtype=np.float64)
    y = np.asarray(y_km2a, dtype=np.float64)
    gx = -y
    gy = x
    if np.ndim(gx) == 0:
        return float(gx), float(gy)
    return gx, gy


def wcda_xy_to_en(
    x_wcda: np.ndarray | float,
    y_wcda: np.ndarray | float,
    z_wcda: np.ndarray | float = 0.0,
):
    x = np.asarray(x_wcda, dtype=np.float64)
    y = np.asarray(y_wcda, dtype=np.float64)
    z = np.asarray(z_wcda, dtype=np.float64)
    ca = np.cos(WCDA_ALPHA_RAD)
    sa = np.sin(WCDA_ALPHA_RAD)
    east = (x * ca - y * sa) + WCDA_CENTER_XYZ[0]
    north = (y * ca + x * sa) + WCDA_CENTER_XYZ[1]
    _ = z + WCDA_CENTER_XYZ[2]
    if np.ndim(east) == 0:
        return float(east), float(north)
    return east, north


def load_detector_positions(path: Union[str, Path]) -> np.ndarray:
    """Read a KM2A ED/MD position text file with rows: id x y z."""

    file_path = str(path)
    if file_path in _DETECTOR_POS_CACHE:
        return _DETECTOR_POS_CACHE[file_path]
    rows = []
    with open(file_path, "r", encoding="utf-8", errors="replace") as handle:
        next(handle, None)
        for line in handle:
            parts = line.split()
            if len(parts) < 4:
                continue
            try:
                _ = int(float(parts[0]))
                x = float(parts[1])
                y = float(parts[2])
                z = float(parts[3])
            except ValueError:
                continue
            if z == 0.0:
                continue
            rows.append((x, y, z))
    arr = np.asarray(rows, dtype=np.float64)
    _DETECTOR_POS_CACHE[file_path] = arr
    return arr


def _wcda_rect_points(cx: float, cy: float, sx: float, sy: float) -> List[Tuple[float, float]]:
    corners = [
        (cx - sx / 2.0, cy - sy / 2.0),
        (cx + sx / 2.0, cy - sy / 2.0),
        (cx + sx / 2.0, cy + sy / 2.0),
        (cx - sx / 2.0, cy + sy / 2.0),
    ]
    return [wcda_xy_to_en(x, y) for x, y in corners]


def _wcda_station_specs() -> List[Tuple[str, float, float, float, float, str]]:
    return [
        ("WCDA Pond 1", -76.6, -57.8, 150.0, 150.0, "#2b8cbe"),
        ("WCDA Pond 2", 76.6, -57.8, 150.0, 150.0, "#41ab5d"),
        ("WCDA Pond 3", 1.2, 77.8, 300.0, 110.0, "#f03b20"),
    ]


def draw_lhaaso_background(
    ax: plt.Axes,
    *,
    ed_pos_file: Optional[Union[str, Path]] = None,
    md_pos_file: Optional[Union[str, Path]] = None,
    set_limits: bool = False,
    show_legend: bool = False,
) -> Dict[str, Any]:
    """Draw the bundled KM2A ED/MD detector map and WCDA ponds."""

    ed_fp = ed_pos_file or DEFAULT_KM2A_ED_POS_FILE
    md_fp = md_pos_file or DEFAULT_KM2A_MD_POS_FILE
    ed = load_detector_positions(ed_fp)
    md = load_detector_positions(md_fp)
    all_x: List[float] = []
    all_y: List[float] = []

    if ed.size:
        ax.scatter(
            ed[:, 0],
            ed[:, 1],
            s=4,
            c="#9ca3af",
            alpha=0.30,
            linewidths=0,
            label=f"KM2A ED ({len(ed)})",
            zorder=0,
        )
        all_x.extend(ed[:, 0].tolist())
        all_y.extend(ed[:, 1].tolist())
    if md.size:
        ax.scatter(
            md[:, 0],
            md[:, 1],
            s=9,
            c="none",
            edgecolors="#4b5563",
            alpha=0.38,
            linewidths=0.5,
            label=f"KM2A MD ({len(md)})",
            zorder=0.5,
        )
        all_x.extend(md[:, 0].tolist())
        all_y.extend(md[:, 1].tolist())

    for _, cx, cy, sx, sy, color in _wcda_station_specs():
        pts = _wcda_rect_points(cx, cy, sx, sy)
        all_x.extend([p[0] for p in pts])
        all_y.extend([p[1] for p in pts])
        ax.add_patch(
            Polygon(
                pts,
                closed=True,
                facecolor=color,
                edgecolor="black",
                alpha=0.20,
                lw=1.2,
                zorder=0.7,
            )
        )

    outer = [wcda_xy_to_en(x, y) for x, y in [(-155, -135), (155, -135), (155, 135), (-155, 135)]]
    all_x.extend([p[0] for p in outer])
    all_y.extend([p[1] for p in outer])
    ax.add_patch(
        Polygon(
            outer,
            closed=True,
            fill=False,
            edgecolor="#6a3d9a",
            lw=1.4,
            ls="--",
            label="WCDA outer 310 x 270 m",
            zorder=0.8,
        )
    )

    if set_limits and all_x and all_y:
        ax.set_xlim(min(all_x) - 45, max(all_x) + 45)
        ax.set_ylim(min(all_y) - 45, max(all_y) + 45)
    if show_legend:
        ax.legend(loc="upper right", fontsize=8, frameon=True)
    return {
        "ed": ed,
        "md": md,
        "ed_pos_file": str(ed_fp),
        "md_pos_file": str(md_fp),
    }


def plot_lhaaso_background(
    *,
    ed_pos_file: Optional[Union[str, Path]] = None,
    md_pos_file: Optional[Union[str, Path]] = None,
    show: bool = True,
):
    fig, ax = plt.subplots(figsize=(9.0, 8.2))
    draw_lhaaso_background(
        ax,
        ed_pos_file=ed_pos_file,
        md_pos_file=md_pos_file,
        set_limits=True,
        show_legend=True,
    )
    ax.set_aspect("equal", adjustable="box")
    ax.set_xlabel("x [m]")
    ax.set_ylabel("y [m]")
    ax.set_title("LHAASO Array Map")
    ax.grid(True, ls=":", lw=0.6, alpha=0.55)
    fig.tight_layout()
    if show:
        plt.show()
    else:
        plt.close(fig)
    return fig, ax

