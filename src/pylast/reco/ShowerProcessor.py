from ..helper import ShowerProcessor as CShowerProcessor  # Consistent naming
import json
from .DispReconstructor import DispReconstructor

class ShowerProcessor:
    C_RECONSTRUCTORS = ["HillasReconstructor"]  # Use constants for clarity
    PY_RECONSTRUCTORS = ["DispReconstructor"]

    def __init__(self, subarray, config_str=None):
        self.subarray = subarray
        self.c_reconstructor_config = {}
        self.py_reconstructor_configs = {}  # Store Python configs separately
        self.c_shower_processor = None  # Initialize to None
        self.disp_reconstructor = None

        if config_str:
            self._parse_config(config_str)

        # Only initialize c_shower_processor if we have C reconstructor configs
        if self.c_reconstructor_config:
            self.c_shower_processor = CShowerProcessor(subarray, config_str)
        if self.py_reconstructor_configs:
            if "DispReconstructor" in self.py_reconstructor_configs:
                self.disp_reconstructor = DispReconstructor(subarray, json.dumps(self.py_reconstructor_configs["DispReconstructor"]))
    def _parse_config(self, config_str):
        try:
            config = json.loads(config_str)
        except json.JSONDecodeError:
            raise ValueError("Invalid JSON configuration string.")

        for name, reco_config in config.items():
            if name in self.C_RECONSTRUCTORS:
                self.c_reconstructor_config[name] = reco_config
            elif name in self.PY_RECONSTRUCTORS:
                self.py_reconstructor_configs[name] = reco_config  # Store Python configs
            else:
                # Handle unknown reconstructors (log, raise exception, or ignore)
                print(f"Warning: Unknown reconstructor '{name}' in configuration.")
                # Or: raise ValueError(f"Unknown reconstructor: {name}")

    def __call__(self, event):
        """Processes an event using the configured reconstructors."""
        if self.c_shower_processor:
            self.c_shower_processor(event)
        if self.disp_reconstructor:
            self.disp_reconstructor(event)
        # Add logic here to use self.py_reconstructor_configs to process the event
        # with Python-based reconstructors.  This part is crucial and was missing
        # from the original code.  Example (you'll need to adapt this):
        # for name, config in self.py_reconstructor_configs.items():
        #     reconstructor = self._get_py_reconstructor(name, config)
        #     reconstructor.process(event)

    def _get_py_reconstructor(self, name, config):
        """
        Factory method to create and return a Python reconstructor instance.
        This is a placeholder; you'll need to implement the actual instantiation
        logic based on your Python reconstructor classes.
        """
        if name == "DispReconstructor":
            # Example: return DispReconstructor(self.subarray, **config)
            #  You'll need to define a DispReconstructor class.
            pass  # Replace with actual instantiation
        else:
            raise ValueError(f"Unsupported Python reconstructor: {name}")

    # Remove __call__ and use process_event instead
    # def __call__(self, event):
    #     self.process_event(event)
