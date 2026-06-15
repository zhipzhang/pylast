"""Event-level plotting helpers for pylast event sources."""

from __future__ import annotations

from pathlib import Path
from os import PathLike

import numpy as np

from pylast.io import LactEventSource

from .event_visualizer import EventVisualizer


def _looks_like_path(value) -> bool:
    return isinstance(value, (str, PathLike, Path))


def _visualizer_from(source=None, visualizer=None):
    if visualizer is not None:
        return visualizer
    if source is None:
        raise ValueError("source or visualizer is required when plotting an event")
    return EventVisualizer(source)


def plot_event_cores(
    event=None,
    *,
    source=None,
    visualizer=None,
    root_file: str | PathLike[str] | None = None,
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    output_path: str | PathLike[str] | None = None,
    include_non_triggered: bool = False,
    show: bool | None = None,
):
    """Draw the LACT array/core view from an already loaded event.

    Pass ``visualizer=EventVisualizer(source)`` or ``source=source``.  The
    optional ``root_file`` path is retained only as a compatibility shortcut;
    notebooks that process DL1/DL2 should pass the in-memory event so the plot
    sees the same calibrated/reconstructed object.
    """

    if root_file is None and _looks_like_path(event):
        root_file = event
        event = None
    if root_file is not None:
        return plot_root_event_cores(
            root_file=root_file,
            event_index=event_index,
            max_events=max_events,
            image_level=image_level,
            output_path=output_path,
            include_non_triggered=include_non_triggered,
            show=show,
        )
    if event is None:
        raise ValueError("event is required")

    if show is None:
        show = output_path is None
    visualizer = _visualizer_from(source=source, visualizer=visualizer)
    figure, axis = visualizer.plot_event_cores(
        event,
        output_path=str(output_path) if output_path is not None else None,
        image_level=image_level,
        include_non_triggered=include_non_triggered,
        show=show,
    )
    return {
        "event": event,
        "visualizer": visualizer,
        "figure": figure,
        "axis": axis,
        "path": Path(output_path) if output_path is not None else None,
    }


plot_event_core = plot_event_cores


def plot_event_sdp_planes(
    event=None,
    *,
    source=None,
    visualizer=None,
    root_file: str | PathLike[str] | None = None,
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    output_path: str | PathLike[str] | None = None,
    include_non_triggered: bool = False,
    show: bool | None = None,
):
    """Draw triggered telescope SDP planes from an already loaded event."""

    if root_file is None and _looks_like_path(event):
        root_file = event
        event = None
    if root_file is not None:
        return plot_root_event_sdp_planes(
            root_file=root_file,
            event_index=event_index,
            max_events=max_events,
            image_level=image_level,
            output_path=output_path,
            include_non_triggered=include_non_triggered,
            show=show,
        )
    if event is None:
        raise ValueError("event is required")

    if show is None:
        show = output_path is None
    visualizer = _visualizer_from(source=source, visualizer=visualizer)
    figure, axis = visualizer.plot_event_sdp_planes(
        event,
        output_path=str(output_path) if output_path is not None else None,
        image_level=image_level,
        include_non_triggered=include_non_triggered,
        show=show,
    )
    return {
        "event": event,
        "visualizer": visualizer,
        "figure": figure,
        "axis": axis,
        "path": Path(output_path) if output_path is not None else None,
    }


def plot_event_sdp_planes_3d(
    event=None,
    *,
    source=None,
    visualizer=None,
    root_file: str | PathLike[str] | None = None,
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    output_path: str | PathLike[str] | None = None,
    include_non_triggered: bool = False,
    z_max: float = 1200.0,
    show_reco: bool = True,
    reconstructor: str = "HillasReconstructor",
    show: bool | None = None,
):
    """Draw a 3D SDP diagnostic from an already loaded event."""

    if root_file is None and _looks_like_path(event):
        root_file = event
        event = None
    if root_file is not None:
        return plot_root_event_sdp_planes_3d(
            root_file=root_file,
            event_index=event_index,
            max_events=max_events,
            image_level=image_level,
            output_path=output_path,
            include_non_triggered=include_non_triggered,
            z_max=z_max,
            show_reco=show_reco,
            reconstructor=reconstructor,
            show=show,
        )
    if event is None:
        raise ValueError("event is required")

    if show is None:
        show = output_path is None
    visualizer = _visualizer_from(source=source, visualizer=visualizer)
    figure, axis = visualizer.plot_event_sdp_planes_3d(
        event,
        output_path=str(output_path) if output_path is not None else None,
        image_level=image_level,
        include_non_triggered=include_non_triggered,
        z_max=z_max,
        show_reco=show_reco,
        reconstructor=reconstructor,
        show=show,
    )
    return {
        "event": event,
        "visualizer": visualizer,
        "figure": figure,
        "axis": axis,
        "path": Path(output_path) if output_path is not None else None,
    }


def plot_event_cameras(
    event=None,
    *,
    source=None,
    visualizer=None,
    root_file: str | PathLike[str] | None = None,
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    show_hillas: bool = False,
    only_hillas_tels: bool = False,
    show_ideal_position: bool = False,
    output_path: str | PathLike[str] | None = None,
    include_non_triggered: bool = False,
    show: bool | None = None,
):
    """Draw camera images from an already loaded event."""

    if root_file is None and _looks_like_path(event):
        root_file = event
        event = None
    if root_file is not None:
        return plot_root_event_cameras(
            root_file=root_file,
            event_index=event_index,
            max_events=max_events,
            image_level=image_level,
            show_hillas=show_hillas,
            only_hillas_tels=only_hillas_tels,
            show_ideal_position=show_ideal_position,
            output_path=output_path,
            include_non_triggered=include_non_triggered,
            show=show,
        )
    if event is None:
        raise ValueError("event is required")

    if show is None:
        show = output_path is None
    visualizer = _visualizer_from(source=source, visualizer=visualizer)
    figure, axes = visualizer.plot_event(
        event,
        output_path=str(output_path) if output_path is not None else None,
        image_level=image_level,
        show_hillas=show_hillas,
        only_hillas_tels=only_hillas_tels,
        include_non_triggered=include_non_triggered,
        show_ideal_position=show_ideal_position,
        show=show,
    )
    return {
        "event": event,
        "visualizer": visualizer,
        "figure": figure,
        "axes": axes,
        "path": Path(output_path) if output_path is not None else None,
    }


def plot_raw_images(
    event=None,
    *,
    source=None,
    visualizer=None,
    root_file: str | PathLike[str] | None = None,
    event_index: int = 0,
    max_events: int = -1,
    output_path: str | PathLike[str] | None = None,
    include_non_triggered: bool = False,
    show: bool | None = None,
):
    """Draw raw integrated camera images from an event.

    Internally this reads pylast's calibrated image layer. For simtelarray
    inputs, run ``Calibrator(event)`` first; LACT ROOT inputs already provide
    this image.
    """

    return plot_event_cameras(
        event,
        source=source,
        visualizer=visualizer,
        root_file=root_file,
        event_index=event_index,
        max_events=max_events,
        image_level="dl0",
        show_hillas=False,
        output_path=output_path,
        include_non_triggered=include_non_triggered,
        show=show,
    )


def plot_clean_images(
    event=None,
    *,
    source=None,
    visualizer=None,
    root_file: str | PathLike[str] | None = None,
    event_index: int = 0,
    max_events: int = -1,
    output_path: str | PathLike[str] | None = None,
    include_non_triggered: bool = False,
    show_hillas: bool = True,
    only_hillas_tels: bool = False,
    show_ideal_position: bool = False,
    show: bool | None = None,
):
    """Draw cleaned camera images from an event.

    Run ``ImageProcessor(event)`` before calling this helper.
    """

    return plot_event_cameras(
        event,
        source=source,
        visualizer=visualizer,
        root_file=root_file,
        event_index=event_index,
        max_events=max_events,
        image_level="dl1",
        show_hillas=show_hillas,
        only_hillas_tels=only_hillas_tels,
        show_ideal_position=show_ideal_position,
        output_path=output_path,
        include_non_triggered=include_non_triggered,
        show=show,
    )


def plot_root_event_cores(
    root_file: str | PathLike[str],
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    output_path: str | PathLike[str] | None = None,
    include_non_triggered: bool = False,
    show: bool | None = None,
):
    """Read one LACT ROOT event and draw the array/core view."""

    source = LactEventSource(str(root_file), max_events=max_events)
    event = source[event_index]
    result = plot_event_cores(
        event,
        source=source,
        image_level=image_level,
        output_path=output_path,
        include_non_triggered=include_non_triggered,
        show=show,
    )
    result["source"] = source
    return result


def plot_root_event_sdp_planes(
    root_file: str | PathLike[str],
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    output_path: str | PathLike[str] | None = None,
    include_non_triggered: bool = False,
    show: bool | None = None,
):
    """Read one LACT ROOT event and draw triggered telescope SDP planes."""

    source = LactEventSource(str(root_file), max_events=max_events)
    event = source[event_index]
    result = plot_event_sdp_planes(
        event,
        source=source,
        image_level=image_level,
        output_path=output_path,
        include_non_triggered=include_non_triggered,
        show=show,
    )
    result["source"] = source
    return result


def plot_root_event_sdp_planes_3d(
    root_file: str | PathLike[str],
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    output_path: str | PathLike[str] | None = None,
    include_non_triggered: bool = False,
    z_max: float = 1200.0,
    show_reco: bool = True,
    reconstructor: str = "HillasReconstructor",
    show: bool | None = None,
):
    """Read one LACT ROOT event and draw a 3D SDP diagnostic."""

    source = LactEventSource(str(root_file), max_events=max_events)
    event = source[event_index]
    result = plot_event_sdp_planes_3d(
        event,
        source=source,
        image_level=image_level,
        output_path=output_path,
        include_non_triggered=include_non_triggered,
        z_max=z_max,
        show_reco=show_reco,
        reconstructor=reconstructor,
        show=show,
    )
    result["source"] = source
    return result


def plot_root_event_cameras(
    root_file: str | PathLike[str],
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    show_hillas: bool = False,
    only_hillas_tels: bool = False,
    show_ideal_position: bool = False,
    output_path: str | PathLike[str] | None = None,
    include_non_triggered: bool = False,
    show: bool | None = None,
):
    """Read one LACT ROOT event and draw camera images."""

    source = LactEventSource(str(root_file), max_events=max_events)
    event = source[event_index]
    result = plot_event_cameras(
        event,
        source=source,
        image_level=image_level,
        show_hillas=show_hillas,
        only_hillas_tels=only_hillas_tels,
        show_ideal_position=show_ideal_position,
        output_path=output_path,
        include_non_triggered=include_non_triggered,
        show=show,
    )
    result["source"] = source
    return result


def hillas_parameter_rows(event):
    """Return per-telescope Hillas parameters already stored on ``event.dl1``."""

    rows = []
    dl1 = getattr(event, "dl1", None)
    tels = getattr(dl1, "tels", {}) if dl1 is not None else {}
    for tel_id, tel in sorted(tels.items()):
        image_parameters = getattr(tel, "image_parameters", None)
        hillas = getattr(image_parameters, "hillas", None)
        if hillas is None:
            continue
        image = np.asarray(getattr(tel, "image", []), dtype=float)
        mask = np.asarray(getattr(tel, "mask", np.ones_like(image)), dtype=bool)
        rows.append(
            {
                "tel_id": int(tel_id),
                "intensity": float(np.sum(image[mask])) if image.size else np.nan,
                "length_rad": float(getattr(hillas, "length", np.nan)),
                "width_rad": float(getattr(hillas, "width", np.nan)),
                "psi_rad": float(getattr(hillas, "psi", np.nan)),
                "x_rad": float(getattr(hillas, "x", getattr(hillas, "cog_x", np.nan))),
                "y_rad": float(getattr(hillas, "y", getattr(hillas, "cog_y", np.nan))),
            }
        )
    return rows


def _angular_separation_rad(alt_a, az_a, alt_b, az_b):
    sin_alt_a, sin_alt_b = np.sin(alt_a), np.sin(alt_b)
    cos_alt_a, cos_alt_b = np.cos(alt_a), np.cos(alt_b)
    cos_angle = sin_alt_a * sin_alt_b + cos_alt_a * cos_alt_b * np.cos(az_a - az_b)
    return float(np.arccos(np.clip(cos_angle, -1.0, 1.0)))


def reconstruction_summary(event, reconstructor: str = "HillasReconstructor"):
    """Return truth and DL2 geometry reconstruction values from an event."""

    shower = getattr(getattr(event, "simulation", None), "shower", None)
    geometry = getattr(getattr(event, "dl2", None), "geometry", {}) or {}
    reco = geometry.get(reconstructor)
    summary = {
        "event_id": getattr(event, "event_id", getattr(event, "count", None)),
        "reconstructor": reconstructor,
        "is_valid": bool(getattr(reco, "is_valid", False)) if reco is not None else False,
        "n_hillas_telescopes": len(hillas_parameter_rows(event)),
    }

    if shower is not None:
        true_alt = float(getattr(shower, "alt", np.nan))
        true_az = float(getattr(shower, "az", np.nan))
        true_core_x = float(getattr(shower, "core_x", np.nan))
        true_core_y = float(getattr(shower, "core_y", np.nan))
        summary.update(
            {
                "true_alt_deg": float(np.degrees(true_alt)),
                "true_az_deg": float(np.degrees(true_az)),
                "true_zenith_deg": float(90.0 - np.degrees(true_alt)),
                "true_core_x_m": true_core_x,
                "true_core_y_m": true_core_y,
            }
        )
    else:
        true_alt = true_az = np.nan

    if reco is not None:
        reco_alt = float(getattr(reco, "alt", np.nan))
        reco_az = float(getattr(reco, "az", np.nan))
        summary.update(
            {
                "reco_alt_deg": float(np.degrees(reco_alt)) if np.isfinite(reco_alt) else np.nan,
                "reco_az_deg": float(np.degrees(reco_az)) if np.isfinite(reco_az) else np.nan,
                "reco_zenith_deg": float(90.0 - np.degrees(reco_alt)) if np.isfinite(reco_alt) else np.nan,
                "reco_core_x_m": float(getattr(reco, "core_x", np.nan)),
                "reco_core_y_m": float(getattr(reco, "core_y", np.nan)),
                "direction_error_deg": float(np.degrees(getattr(reco, "direction_error", np.nan))),
            }
        )
        if all(np.isfinite(v) for v in (true_alt, true_az, reco_alt, reco_az)):
            summary["truth_reco_sep_deg"] = float(np.degrees(_angular_separation_rad(reco_alt, reco_az, true_alt, true_az)))

    return summary


def plot_event_quicklook(
    root_file: str | PathLike[str],
    output_dir: str | PathLike[str] | None = None,
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    plot_gathered: bool = True,
    plot_telescopes: bool = True,
    plot_event: bool = True,
    include_non_triggered: bool = False,
    show: bool | None = None,
):
    """Read a LACT ROOT file with ``LactEventSource`` and make standard plots.

    Parameters
    ----------
    root_file:
        Path to a LACT_sim ``lact_event_root_v1`` ROOT file.
    output_dir:
        Directory where PNG files will be written. If omitted, plots are not
        saved and are displayed directly, which is convenient in notebooks.
    event_index:
        Random-access event index to plot.
    max_events:
        Maximum events exposed by the source. ``-1`` means all events.
    image_level:
        pylast image level for plotting. For LACT ROOT files, ``dl0`` is the
        integrated p.e. image derived from ``observations.image_pe``.
    plot_gathered, plot_telescopes, plot_event:
        Select which standard event plots to write.
    include_non_triggered:
        If false, array and camera plots show only telescope images marked as
        camera-triggered by the LACT ROOT observations. If true, non-triggered
        telescope images with nonzero p.e. are also shown.
    show:
        Whether matplotlib should display figures interactively. By default,
        plots are displayed when ``output_dir`` is omitted and closed when
        files are saved.

    Returns
    -------
    dict
        Contains the source, event, visualizer, written plot paths, and figure
        handles.
    """

    outdir = Path(output_dir) if output_dir is not None else None
    if outdir is not None:
        outdir.mkdir(parents=True, exist_ok=True)
    if show is None:
        show = outdir is None

    source = LactEventSource(str(root_file), max_events=max_events)
    event = source[event_index]
    visualizer = EventVisualizer(source)

    event_id = getattr(event, "event_id", event_index)
    paths = {}
    figures = {}
    axes = {}
    if plot_gathered:
        path = outdir / f"event_{event_id}_gathered_{image_level}.png" if outdir is not None else None
        figure, axis = visualizer.plot_gathered_event(
            event,
            output_path=str(path) if path is not None else None,
            image_level=image_level,
            show_hillas=False,
            include_non_triggered=include_non_triggered,
            show=show,
        )
        if path is not None:
            paths["gathered"] = path
        figures["gathered"] = figure
        axes["gathered"] = axis

    if plot_telescopes:
        path = outdir / f"event_{event_id}_telescopes_{image_level}.png" if outdir is not None else None
        figure, axis = visualizer.plot_telescopes(
            event,
            output_path=str(path) if path is not None else None,
            image_level=image_level,
            include_non_triggered=include_non_triggered,
            show=show,
        )
        if path is not None:
            paths["telescopes"] = path
        figures["telescopes"] = figure
        axes["telescopes"] = axis

    if plot_event:
        path = outdir / f"event_{event_id}_cameras_{image_level}.png" if outdir is not None else None
        figure, axis = visualizer.plot_event(
            event,
            output_path=str(path) if path is not None else None,
            image_level=image_level,
            show_hillas=False,
            include_non_triggered=include_non_triggered,
            show=show,
        )
        if path is not None:
            paths["event"] = path
        figures["event"] = figure
        axes["event"] = axis

    return {
        "source": source,
        "event": event,
        "visualizer": visualizer,
        "paths": paths,
        "figures": figures,
        "axes": axes,
    }


plot_lact_root_quicklook = plot_event_quicklook


def show_lact_root_event(
    root_file: str | PathLike[str],
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    kind: str = "telescopes",
    include_non_triggered: bool = False,
    show: bool = True,
):
    """Display one LACT ROOT event directly in a notebook.

    This is a compatibility wrapper. Prefer ``plot_event_quicklook`` with
    ``output_dir=None`` for new notebooks.
    """

    if kind not in {"telescopes", "cameras", "gathered"}:
        raise ValueError("kind must be one of: telescopes, cameras, gathered")
    return plot_event_quicklook(
        root_file=root_file,
        output_dir=None,
        event_index=event_index,
        max_events=max_events,
        image_level=image_level,
        plot_gathered=kind == "gathered",
        plot_telescopes=kind == "telescopes",
        plot_event=kind == "cameras",
        include_non_triggered=include_non_triggered,
        show=show,
    )
