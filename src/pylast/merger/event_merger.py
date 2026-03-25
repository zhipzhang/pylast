try:
    import pykm2arec
except ImportError:
    raise ImportError("pykm2arec is not installed")

from pykm2arec import KM2AEventSource, KM2AMCEvent
from pylast.io import RootEventSource, SimtelEventSource

class EventMerger:
    def __init__(self, lact_file_path: str, km2a_file_path: str):
        self.lact_file_path = lact_file_path
        self.km2a_file_path = km2a_file_path
        self.km2a_event_source = KM2AEventSource(km2a_file_path)
        try:
            self.lact_event_source = RootEventSource(lact_file_path)
        except:
            self.lact_event_source = SimtelEventSource(lact_file_path)
    
    @classmethod
    def from_source(cls, lact_event_source, km2a_event_source):
        """
        Create an EventMerger from already-initialized event source objects.
        lact_event_source: an instance of RootEventSource or SimtelEventSource or compatible object
        km2a_event_source: an instance of KM2AEventSource
        """
        # Create dummy file path placeholders, if needed
        obj = cls.__new__(cls)
        obj.lact_file_path = getattr(lact_event_source, "filename", None)
        obj.km2a_file_path = getattr(km2a_event_source, "filename", None)
        obj.lact_event_source = lact_event_source
        obj.km2a_event_source = km2a_event_source
        return obj

    def build_km2a_mapping(self):
        """
        Build a mapping from (energy, core_x, core_y) tuple to i_event index.
        Note:
            The keys are exact floating-point values; for robust lookup, 
            consider rounding or tolerances as appropriate for your matching use case.
        """
        self._mapping_energy_core_to_event_id = {}

        for i_event, event in enumerate(self.km2a_event_source):
            energy = round(event.energy/1000, 2)
            core_x = round(event.corex, 2)
            core_y = round(event.corey, 2)

            key = (energy, core_x, core_y)
            print(f'build mapping: {key} -> {i_event}')
            self._mapping_energy_core_to_event_id[key] = i_event

    def __iter__(self):
        """
        Iterate over the lact_event_source, yield (lact_event, km2a_event) for events
        with a matching (energy, core_x, core_y) found in km2a source.
        """
        if not hasattr(self, "_mapping_energy_core_to_event_id"):
            self.build_km2a_mapping()
        
        for lact_event in self.lact_event_source:
            # Attempt to fetch these from LACT event
            # This assumes that the lact_event object has attributes or properties
            # named 'energy', 'core_x', and 'core_y'.
            try:
                energy = round(lact_event.simulation.shower.energy, 2)
                core_x = round(lact_event.simulation.shower.core_x, 2)
                core_y = round(lact_event.simulation.shower.core_y, 2)
            except AttributeError:
                # Try alternative names for attributes if needed
                raise AttributeError("lact_event is missing 'simulation.shower.energy', 'simulation.shower.core_x', or 'simulation.shower.core_y' attributes")
            
            key = (energy, core_x, core_y)
            print("finding key: ", key)
            km2a_index = self._mapping_energy_core_to_event_id.get(key, None)
            if km2a_index is None:
                raise ValueError(f"No matched KM2A event found for energy {energy}, core_x {core_x}, core_y {core_y}")
            # Get the km2a event by index
            km2a_event = self.km2a_event_source.get_event(km2a_index)
            yield (lact_event, km2a_event)