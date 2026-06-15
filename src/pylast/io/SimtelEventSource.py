from ..helper import SimtelEventSource as _NativeSimtelEventSource


def _tel_ids_from(container):
    tels = getattr(container, "tels", None)
    if not tels:
        return ()
    return tuple(sorted(int(tel_id) for tel_id in tels))


def _snapshot_triggered_tels(event):
    simulation = getattr(event, "simulation", None)
    triggered = getattr(simulation, "triggered_tels", None)
    if triggered is not None:
        triggered_tels = tuple(sorted(int(tel_id) for tel_id in triggered))
    else:
        triggered_tels = _tel_ids_from(getattr(event, "r1", None))
        if not triggered_tels:
            triggered_tels = _tel_ids_from(getattr(event, "r0", None))

    if triggered_tels:
        try:
            setattr(event, "_pylast_original_triggered_tels", triggered_tels)
        except Exception:
            pass
    return event


class SimtelEventSource:
    """Python adapter around the native simtelarray event source.

    Some native simtel bindings expose triggered telescopes only through the
    waveform levels instead of ``event.simulation.triggered_tels``. When the
    event object supports dynamic attributes, preserve a read-time copy so
    notebook diagnostics and visualization helpers use the same telescope set.
    """

    def __init__(self, *args, **kwargs):
        self._source = _NativeSimtelEventSource(*args, **kwargs)

    def __getattr__(self, name):
        return getattr(self._source, name)

    def __iter__(self):
        for event in self._source:
            yield _snapshot_triggered_tels(event)

    def __getitem__(self, item):
        return _snapshot_triggered_tels(self._source[item])

    def __len__(self):
        return len(self._source)
