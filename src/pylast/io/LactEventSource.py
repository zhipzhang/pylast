from ..helper import LactEventSource as _NativeLactEventSource


def _snapshot_triggered_tels(event):
    simulation = getattr(event, "simulation", None)
    triggered = getattr(simulation, "triggered_tels", None)
    if triggered is not None:
        triggered_tels = tuple(sorted(int(tel_id) for tel_id in triggered))
        if triggered_tels:
            try:
                setattr(event, "_pylast_original_triggered_tels", triggered_tels)
            except Exception:
                pass
    return event


class LactEventSource:
    """Python adapter around the native LACT ROOT event source.

    The native pylast image processing chain can clear
    ``event.simulation.triggered_tels`` while building DL1 products. Preserve a
    copy at read time so downstream visualization and reconstruction diagnostics
    can still distinguish triggered telescopes from non-triggered telescopes
    with nonzero p.e.
    """

    def __init__(self, *args, **kwargs):
        self._source = _NativeLactEventSource(*args, **kwargs)

    def __getattr__(self, name):
        return getattr(self._source, name)

    def __iter__(self):
        for event in self._source:
            yield _snapshot_triggered_tels(event)

    def __getitem__(self, item):
        return _snapshot_triggered_tels(self._source[item])

    def __len__(self):
        return len(self._source)
