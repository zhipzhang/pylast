"""Quick-look plotting helpers for LACT_sim ROOT files read through pylast."""

from __future__ import annotations

from pathlib import Path
from os import PathLike

from pylast.io import LactEventSource

from .fast_visualizer import EventVisualizer


def plot_lact_root_quicklook(
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
        Select which standard quick-look plots to write.
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

    This is a compatibility wrapper. Prefer ``plot_lact_root_quicklook`` with
    ``output_dir=None`` for new notebooks.
    """

    if kind not in {"telescopes", "cameras", "gathered"}:
        raise ValueError("kind must be one of: telescopes, cameras, gathered")
    return plot_lact_root_quicklook(
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
