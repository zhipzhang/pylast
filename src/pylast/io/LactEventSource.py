from ..helper import LactEventSource as _NativeLactEventSource


def _event_id(event_or_id):
    if isinstance(event_or_id, int):
        return event_or_id
    event_id = getattr(event_or_id, "event_id", None)
    return None if event_id is None else int(event_id)


def _native_triggered_tels(event):
    simulation = getattr(event, "simulation", None)
    triggered = getattr(simulation, "triggered_tels", None)
    if triggered is not None:
        triggered_tels = tuple(sorted(int(tel_id) for tel_id in triggered))
        if triggered_tels:
            return triggered_tels
    return ()


class LactEventSource:
    """Python adapter around the native LACT ROOT event source.

    Exposes a uniform ``get_triggered_tels`` helper. LACT ROOT readout images
    are detector-level data, so the native adapter does not need to put them in
    ``event.simulation`` just to carry trigger state. The observations tree is
    used as the source-level trigger table.
    """

    def __init__(self, *args, **kwargs):
        self._source = _NativeLactEventSource(*args, **kwargs)
        self._triggered_tels_by_event_id = {}
        self._ground_counts_by_event_id = {}
        self._input_filename = getattr(self._source, "input_filename", None)
        if self._input_filename is None and args:
            self._input_filename = str(args[0])

    def __getattr__(self, name):
        return getattr(self._source, name)

    def __iter__(self):
        for event in self._source:
            self._remember_triggered_tels(event)
            yield event

    def __getitem__(self, item):
        event = self._source[item]
        self._remember_triggered_tels(event)
        return event

    def __len__(self):
        return len(self._source)

    def _read_root_triggered_tels(self, event_id):
        filename = self._input_filename
        if filename is None or event_id is None or not str(filename).endswith(".root"):
            return ()
        try:
            import ROOT

            root_file = ROOT.TFile.Open(str(filename))
            if not root_file or root_file.IsZombie():
                return ()
            tree = root_file.Get("observations")
            required = ("event_id", "telescope_id", "triggered")
            if tree is None or any(tree.GetBranch(name) is None for name in required):
                root_file.Close()
                return ()
            triggered = []
            for entry in range(tree.GetEntries()):
                tree.GetEntry(entry)
                if int(tree.event_id) == int(event_id) and bool(tree.triggered):
                    triggered.append(int(tree.telescope_id))
            root_file.Close()
            return tuple(sorted(set(triggered)))
        except Exception:
            return ()

    def _read_root_ground_counts(self, event_id):
        filename = self._input_filename
        if filename is None or event_id is None or not str(filename).endswith(".root"):
            return {}
        try:
            import ROOT

            root_file = ROOT.TFile.Open(str(filename))
            if not root_file or root_file.IsZombie():
                return {}
            tree = root_file.Get("corsika_events")
            required = ("event_id", "ground_gammas", "ground_electrons", "ground_hadrons", "ground_muons")
            if tree is None or any(tree.GetBranch(name) is None for name in required):
                root_file.Close()
                return {}
            for entry in range(tree.GetEntries()):
                tree.GetEntry(entry)
                if int(tree.event_id) == int(event_id):
                    counts = {
                        "ground_gammas": float(tree.ground_gammas),
                        "ground_electrons": float(tree.ground_electrons),
                        "ground_hadrons": float(tree.ground_hadrons),
                        "ground_muons": float(tree.ground_muons),
                    }
                    root_file.Close()
                    return counts
            root_file.Close()
        except Exception:
            return {}
        return {}

    def _remember_triggered_tels(self, event):
        event_id = _event_id(event)
        if event_id is None:
            return ()
        triggered_tels = _native_triggered_tels(event)
        if not triggered_tels:
            triggered_tels = self._read_root_triggered_tels(event_id)
        if triggered_tels:
            self._triggered_tels_by_event_id[event_id] = triggered_tels
        return triggered_tels

    def get_triggered_tels(self, event_or_id):
        event_id = _event_id(event_or_id)
        if event_id is None:
            return []
        if not isinstance(event_or_id, int):
            triggered_tels = _native_triggered_tels(event_or_id)
            if triggered_tels:
                self._triggered_tels_by_event_id[event_id] = triggered_tels
                return list(triggered_tels)
        if event_id not in self._triggered_tels_by_event_id:
            triggered_tels = self._read_root_triggered_tels(event_id)
            if triggered_tels:
                self._triggered_tels_by_event_id[event_id] = triggered_tels
        return list(self._triggered_tels_by_event_id.get(event_id, ()))

    def get_ground_counts(self, event_or_id):
        event_id = _event_id(event_or_id)
        if event_id is None:
            return {}
        if event_id not in self._ground_counts_by_event_id:
            counts = self._read_root_ground_counts(event_id)
            if counts:
                self._ground_counts_by_event_id[event_id] = counts
        return dict(self._ground_counts_by_event_id.get(event_id, {}))
