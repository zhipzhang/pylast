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
            show=show,
        )
        paths["gathered"] = path

    if plot_telescopes:
        path = outdir / f"event_{event_id}_telescopes_{image_level}.png"
        visualizer.plot_telescopes(
            event,
            output_path=str(path),
            image_level=image_level,
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
            show=show,
        )
        paths["event"] = path

    return {
        "source": source,
        "event": event,
        "visualizer": visualizer,
        "paths": paths,
    }
