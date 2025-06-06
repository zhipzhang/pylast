import json
import pickle

from ..helper import MLReconstructor as CMLReconstructor
from ..helper import ReconstructedEnergy as CReconstructedEnergy
import numpy as np
import os
from ..helper import compute_angle_separation

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
            energy_regressor_path = os.path.join(model_directory, "energy_regressor_offset.pkl")
            with open(energy_regressor_path, "rb") as f:
                self.model = pickle.load(f)
        self.check_model()
        self.energy = CReconstructedEnergy()
    def check_model(self):
        if self.model is None:
            raise ValueError("model is not set")
        else:
            self.energy_predictor = self.model

        
    def predict(self, features, offset_angle):
        # Determine which model to use based on offset angle
        if offset_angle < 1.0:
            offset_key = "0_1"
        elif offset_angle < 2.0:
            offset_key = "1_2"
        elif offset_angle < 3.0:
            offset_key = "2_3"
        elif offset_angle < 4.0:
            offset_key = "3_4"
        else:
            offset_key = "4_n"
        
        # Check if features is a batch or single sample
        if isinstance(features, list) or (isinstance(features, np.ndarray) and len(features.shape) > 1 and features.shape[0] > 1):
            # Batch prediction
            predictions = self.energy_predictor[f"energy_regressor_{offset_key}"].predict(features)
            return predictions
        else:
            # Single sample prediction
            prediction = self.energy_predictor[f"energy_regressor_{offset_key}"].predict([features])
            return prediction

    def __call__(self, event):
        super().__call__(event)
        # Calculate average intensity in one pass

        if event.dl2.geometry["HillasReconstructor"].is_valid:
            intensities = np.array([event.dl1.tels[tel_id].image_parameters.hillas.intensity for tel_id in event.dl2.tels])
            self.average_intensity = np.mean(intensities)
            # Pre-allocate arrays
            n_telescopes = len(self.telescopes)
            telescopes_energys = np.zeros(n_telescopes)
            weights = np.zeros(n_telescopes)
            
            # Should be careful that we may not have pointing direction in the event!!
            offset_angle = compute_angle_separation(event.dl2.geometry["HillasReconstructor"].alt, event.dl2.geometry["HillasReconstructor"].az, self.array_pointing_direction.altitude, self.array_pointing_direction.azimuth)
            # Prepare feature arrays for batch prediction if possible
            features_list = []
            for itel, tel_id in enumerate(self.telescopes):
                features = self.get_features(event, tel_id)
                features_list.append(features)
            
            # If the model supports batch prediction, use it
            try:
                # Combine all features into a single batch
                batch_features = np.vstack(features_list)
                batch_energies = self.predict(batch_features, offset_angle)
                

                for itel, tel_id in enumerate(self.telescopes):
                    energy = batch_energies[itel]
                    energy_value = pow(10, energy)
                    telescopes_energys[itel] = energy_value
                    weights[itel] = event.dl1.tels[tel_id].image_parameters.hillas.intensity
                    event.dl2.set_tel_estimate_energy(tel_id, energy_value)
            except:
                # Fall back to individual predictions if batch prediction fails
                for itel, tel_id in enumerate(self.telescopes):
                    features = features_list[itel]
                    energy = self.predict(features, offset_angle)[0]
                    energy_value = pow(10, energy)
                    telescopes_energys[itel] = energy_value
                    weights[itel] = event.dl1.tels[tel_id].image_parameters.hillas.intensity
                    event.dl2.set_tel_estimate_energy(tel_id, energy_value)
            
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
        
        # Extract all parameters at once to reduce attribute lookups
        hillas = dl1_tel.image_parameters.hillas
        leakage = dl1_tel.image_parameters.leakage
        concentration = dl1_tel.image_parameters.concentration
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
            hillas.width/hillas.length,      # hillas_width_length_ratio
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
            self.average_intensity,          # average_intensity
            n_tel,                           # n_tel
        ]
        return np.array(feature_values)
