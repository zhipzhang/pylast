from ..helper import ShowerProcessor as CShowerProcessor  # Consistent naming
import json

class ShowerProcessor:
    C_RECONSTRUCTORS = ["HillasReconstructor"]  # Use constants for clarity
    PY_RECONSTRUCTORS = []

    def __init__(self, subarray, config_str=None):
        self.subarray = subarray
        self.c_reconstructor_config = {}
        self.py_reconstructor_configs = {}  # Store Python configs separately
        self.c_shower_processor = None  # Initialize to None
        self.disp_reconstructor = None
        self.disp_stereo_reconstructor = None
        self.mle_reconstructor = None
        self.hillas_weighted_reconstructor = None
        self.ml_particle_classifier = None
        if config_str:
            self._parse_config(config_str)

        # Only initialize c_shower_processor if we have C reconstructor configs
        if self.c_reconstructor_config:
            self.c_shower_processor = CShowerProcessor(subarray, config_str)
    def _parse_config(self, config_str):
        try:
            config = json.loads(config_str)
            config = config.get("ShowerProcessor", config)
        except json.JSONDecodeError:
            raise ValueError("Invalid JSON configuration string.")
        for reconstruction_type in config["GeometryReconstructionTypes"]:
            if reconstruction_type in self.C_RECONSTRUCTORS:
                self.c_reconstructor_config[reconstruction_type] = config[reconstruction_type]
            elif reconstruction_type in self.PY_RECONSTRUCTORS:
                self.py_reconstructor_configs[reconstruction_type] = config[reconstruction_type]
            else:
                raise ValueError(f"Unknown reconstruction type: {reconstruction_type}")
    def __call__(self, event):
        """Processes an event using the configured reconstructors."""
        if self.c_shower_processor:
            self.c_shower_processor(event)
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
