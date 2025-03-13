import json
import pickle
from ..helper import GeometryReconstructor as CGeometryReconstructor
import numpy as np
class DispReconstructor(CGeometryReconstructor):
    def __init__(self, subarray, config_str=None):
        super().__init__(subarray, config_str)
        self.name = "DispReconstructor"
        if config_str is None:
            self.config = {}
        else:
            self.config = json.loads(config_str)
        if("model_path" in self.config):
            with open(self.config["model_path"], "rb") as f:
                self.model = pickle.load(f)
        else:
            raise ValueError("model_path is not set")
        self.check_model()
    def check_model(self):
        if self.model is None:
            raise ValueError("model is not set")
        else:
            self.disp_predictor = self.model["disp_model"]
            self.disp_sign_predictor = self.model["sign_model"]
    def __call__(self, event):
        # Make sure we have a dl2 event
        super().__call__(event)
        if(len(self.hillas_dicts) < 2):
            self.geometry.is_valid = False
            event.dl2.add_geometry(self.name, self.geometry)
            return
        x_rec = np.zeros(len(self.telescopes))
        y_rec = np.zeros(len(self.telescopes))
        weights = np.zeros(len(self.telescopes))
        for i, tel_id in enumerate(self.telescopes):
            # We have to one by one call the predictors
            features = self.get_features(event, tel_id)
            disp = self.disp_predictor.predict(features)
            sign = self.disp_sign_predictor.predict(features)
            event.dl2.tels[tel_id].disp = disp * sign
            hillas_x = self.hillas_dicts[tel_id].x
            hillas_y = self.hillas_dicts[tel_id].y
            x_rec[i] = hillas_x + disp * sign * np.cos(self.hillas_dicts[tel_id].psi)
            y_rec[i] = hillas_y + disp * sign * np.sin(self.hillas_dicts[tel_id].psi)
            weights[i] = self.hillas_dicts[tel_id].intensity
        fov_x = np.average(x_rec, weights=weights)
        fov_y = np.average(y_rec, weights=weights)
        rec_az, rec_alt = self.convert_to_sky(fov_x, fov_y)
        sigma_x = np.sqrt(np.average(np.square(x_rec - fov_x), weights=weights))
        sigma_y = np.sqrt(np.average(np.square(y_rec - fov_y), weights=weights))
        self.geometry.is_valid = True
        self.geometry.az = rec_az
        self.geometry.alt = rec_alt
        self.geometry.alt_uncertainty = sigma_x
        self.geometry.az_uncertainty = sigma_y
        self.geometry.set_telescopes(self.telescopes)
        self.geometry.direction_error = self.compute_angle_separation(event.simulation.shower.az, event.simulation.shower.alt, rec_az, rec_alt)
        event.dl2.add_geometry(self.name, self.geometry)

    def get_features(self, event, tel_id):
        """
        Extract features from the event for the given telescope ID.
        
        Args:
            event: The array event containing DL1 and DL2 data
            tel_id: The telescope ID to extract features for
            
        Returns:
            A list of feature values in the required order
        """
        # Initialize an empty list to store features
        feature_values = []
        
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
        
        # Calculate area (length * width * pi)
        area = hillas.length * hillas.width * 3.14159
        
        # Get impact parameter from DL2 if available
        impact_parameter = dl2_tel.impact.distance
        
        # Collect all features in the required order
        feature_values = [
            event.simulation.shower.energy,
            hillas.intensity,
            hillas.r,
            hillas.length,
            hillas.width,
            hillas.psi,
            leakage.intensity_width_1,
            leakage.intensity_width_2,       # leakage_intensity_width_2
            leakage.pixels_width_1,          # leakage_pixels_width_1
            leakage.pixels_width_2,          # leakage_pixels_width_2
            concentration.concentration_cog,  # concentration_cog
            concentration.concentration_core, # concentration_core
            morphology.n_islands,            # morphology_n_islands
            morphology.n_large_islands,      # morphology_n_large_islands
            morphology.n_medium_islands,     # morphology_n_medium_islands
            morphology.n_pixels,             # morphology_n_pixels
            area,                            # area
            impact_parameter                 # impact_parameter
        ]
        
        return np.array([feature_values])
