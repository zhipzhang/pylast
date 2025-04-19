import json
import pickle

from ..helper import MLReconstructor as CMLReconstructor
from ..helper import ReconstructedEnergy as CReconstructedEnergy
import numpy as np
import os


class MLEnergyReconstructor(CMLReconstructor):
    def __init__(self, config_str=None):
        super().__init__(config_str)
        self.name = "MLEnergyReconstructor"
        if config_str is None:
            self.config = {}
        else:
            self.config = json.loads(config_str)
        if("model_path" in self.config):
            with open(self.config["model_path"], "rb") as f:
                self.model = pickle.load(f)
        else:
            model_directory = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "model")
            energy_regressor_path = os.path.join(model_directory, "energy_regressor.pkl")
            with open(energy_regressor_path, "rb") as f:
                self.model = pickle.load(f)
        self.check_model()
        self.energy = CReconstructedEnergy()
    def check_model(self):
        if self.model is None:
            raise ValueError("model is not set")
        else:
            self.energy_predictor = self.model["energy_regressor"]
    def __call__(self, event):
        super().__call__(event)
        if event.dl2.geometry["HillasReconstructor"].is_valid:
            telescopes_energys = np.zeros(len(self.telescopes))
            weights = np.zeros(len(self.telescopes))
            for itel,tel_id in enumerate(self.telescopes):
                features = self.get_features(event, tel_id)
                energy = self.energy_predictor.predict(features)
                telescopes_energys[itel] = pow(10, energy)
                weights[itel] = event.dl1.tels[tel_id].image_parameters.hillas.intensity
                event.dl2.set_tel_estimate_energy(tel_id, pow(10, energy))
            self.energy.energy_valid = True
            self.energy.estimate_energy = np.average(telescopes_energys, weights=weights)
        else:
            self.energy.energy_valid = False
            self.energy.estimate_energy = 0
        event.dl2.add_energy(self.name, self.energy)

    def get_features(self, event, tel_id):
        """
        Extract features from the event for the given telescope ID.
        
        Args:
            event: The array event containing DL1 and DL2 data
            tel_id: The telescope ID to extract features for
            
        Returns:
            A numpy array of feature values in the required order
        """
        # Get DL1 data for this telescope
        dl1_tel = event.dl1.tels[tel_id]
        
        # Get DL2 data for this telescope (for impact parameter)
        dl2_tel = event.dl2.tels[tel_id]
        
        # Extract Hillas parameters
        hillas = dl1_tel.image_parameters.hillas
        
        # Extract leakage parameters
        leakage = dl1_tel.image_parameters.leakage
        
        # Extract concentration parameters
        concentration = dl1_tel.image_parameters.concentration
        
        # Extract morphology parameters
        morphology = dl1_tel.image_parameters.morphology

        intensity = dl1_tel.image_parameters.intensity
        
        # Get impact parameter from DL2
        impact_parameter = dl2_tel.impact.distance

        n_tel = len(self.telescopes)
        # Collect all features in the required order
        feature_values = [
            impact_parameter,                # rec_impact_parameter
            hillas.length,                   # hillas_length
            hillas.width,                    # hillas_width
            hillas.skewness,                 # hillas_skewness
            hillas.kurtosis,                 # hillas_kurtosis
            hillas.intensity,                # hillas_intensity
            leakage.pixels_width_1,          # leakage_pixels_width_1
            leakage.pixels_width_2,          # leakage_pixels_width_2
            leakage.intensity_width_1,       # leakage_intensity_width_1
            leakage.intensity_width_2,       # leakage_intensity_width_2
            concentration.concentration_cog,  # concentration_cog
            concentration.concentration_core, # concentration_core
            concentration.concentration_pixel,# concentration_pixel
            morphology.n_pixels,             # morphology_num_pixels
            morphology.n_islands,            # morphology_num_islands
            intensity.intensity_max,         # intensity_max
            intensity.intensity_mean,        # intensity_mean
            intensity.intensity_std,         # intensity_std
            n_tel,                           # n_tel
        ]
        return np.array([feature_values])
