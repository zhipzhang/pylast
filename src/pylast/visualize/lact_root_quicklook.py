"""Quick-look plotting helpers for LACT_sim ROOT files read through pylast."""

from __future__ import annotations

from pathlib import Path
from os import PathLike

from pylast.io import LactEventSource

from .fast_visualizer import EventVisualizer


def plot_lact_root_quicklook(
    root_file: str | PathLike[str],
    output_dir: str | PathLike[str],
    event_index: int = 0,
    max_events: int = -1,
    image_level: str = "dl0",
    plot_gathered: bool = True,
    plot_telescopes: bool = True,
    plot_event: bool = True,
    include_non_triggered: bool = False,
    show: bool = False,
):
    """Read a LACT ROOT file with ``LactEventSource`` and save standard plots.

    Parameters
    ----------
    root_file:
        Path to a LACT_sim ``lact_event_root_v1`` ROOT file.
    output_dir:
        Directory where PNG files will be written.
    event_index:
        Random-access event index to plot.
    max_events:
        Maximum events exposed by the source. ``-1`` means all events.
    image_level:
        pylast image level for plotting. For LACT ROOT files, ``dl0`` is the
        integrated p.e. image derived from ``observations.image_pe``.
    plot_gathered, plot_telescopes, plot_event:
        Select which standard quick-look plots to write.
    include_non_triggered:
        If false, array and camera plots show only telescope images marked as
        camera-triggered by the LACT ROOT observations. If true, non-triggered
        telescope images with nonzero p.e. are also shown.
    show:
        Whether matplotlib should display figures interactively.

    Returns
    -------
    dict
        Contains the source, event, visualizer, and written plot paths.
    """

    outdir = Path(output_dir)
    outdir.mkdir(parents=True, exist_ok=True)

    source = LactEventSource(str(root_file), max_events=max_events)
    event = source[event_index]
    visualizer = EventVisualizer(source)

    event_id = getattr(event, "event_id", event_index)
    paths = {}
    if plot_gathered:
        path = outdir / f"event_{event_id}_gathered_{image_level}.png"
        visualizer.plot_gathered_event(
            event,
            output_path=str(path),
            image_level=image_level,
            show_hillas=False,
            include_non_triggered=include_non_triggered,
            show=show,
        )
        paths["gathered"] = path

    if plot_telescopes:
        path = outdir / f"event_{event_id}_telescopes_{image_level}.png"
        visualizer.plot_telescopes(
            event,
            output_path=str(path),
            image_level=image_level,
            include_non_triggered=include_non_triggered,
            show=show,
        )
        paths["telescopes"] = path

    if plot_event:
        path = outdir / f"event_{event_id}_cameras_{image_level}.png"
        visualizer.plot_event(
            event,
            output_path=str(path),
            image_level=image_level,
            show_hillas=False,
            include_non_triggered=include_non_triggered,
            show=show,
        )
        paths["event"] = path

    return {
        "source": source,
        "event": event,
        "visualizer": visualizer,
        "paths": paths,
    }


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

    Examples
    --------
    >>> from pylast.visualize import show_lact_root_event
    >>> show_lact_root_event("lact_events.root", event_index=0)

    Set ``kind="cameras"`` or ``kind="gathered"`` for the camera views.
    """

    source = LactEventSource(str(root_file), max_events=max_events)
    event = source[event_index]
    visualizer = EventVisualizer(source)

    if kind == "telescopes":
        figure, axes = visualizer.plot_telescopes(
            event,
            image_level=image_level,
            include_non_triggered=include_non_triggered,
            show=show,
        )
    elif kind == "cameras":
        figure, axes = visualizer.plot_event(
            event,
            image_level=image_level,
            show_hillas=False,
            include_non_triggered=include_non_triggered,
            show=show,
        )
    elif kind == "gathered":
        figure, axes = visualizer.plot_gathered_event(
            event,
            image_level=image_level,
            show_hillas=False,
            include_non_triggered=include_non_triggered,
            show=show,
        )
    else:
        raise ValueError("kind must be one of: telescopes, cameras, gathered")

    return {
        "source": source,
        "event": event,
        "visualizer": visualizer,
        "figure": figure,
        "axes": axes,
    }
