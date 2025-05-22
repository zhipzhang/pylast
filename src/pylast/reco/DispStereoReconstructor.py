import json
import pickle
import numpy as np
import os
import numba as nb
from ..helper import GeometryReconstructor as CGeometryReconstructor
from ..helper import compute_angle_separation
class DispStereoReconstructor(CGeometryReconstructor):
    def __init__(self, subarray, config_str=None):
        super().__init__(subarray, config_str)
        self.name = "DispStereoReconstructor"
        if config_str is None:
            self.config = {}
        else:
            self.config = json.loads(config_str)
        if("model_path" in self.config):
            with open(self.config["model_path"], "rb") as f:
                self.model = pickle.load(f)
        else:
            model_directory = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "model")
            disp_model_path = os.path.join(model_directory, "disp_model.pkl")
            with open(disp_model_path, "rb") as f:
                self.disp_model = pickle.load(f)
        self.check_model()
        
    def check_model(self):
        if self.disp_model is None:
            raise ValueError("model is not set")
        else:
            self.disp_predictor = self.disp_model
            
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
            predictions = self.disp_predictor[f"disp_predictor_{offset_key}"].predict(features)
            return predictions
        else:
            # Single sample prediction
            prediction = self.disp_predictor[f"disp_predictor_{offset_key}"].predict([features])
            return prediction[0]
            
    def __call__(self, event):
        # Make sure we have a dl2 event
        super().__call__(event)
        if(len(self.hillas_dicts) < 2 or list(event.dl2.energy.values())[0].energy_valid is False):
            self.geometry.is_valid = False
            event.dl2.add_geometry(self.name, self.geometry)
            return
            
        # Create arrays to store all possible telescope combinations
        import itertools
        self.n_tel = len(self.telescopes)
        
        # Generate all possible pairs of telescopes
        tel_combinations = list(itertools.combinations(self.telescopes, 2))
        x_rec = np.zeros(len(tel_combinations))
        y_rec = np.zeros(len(tel_combinations))
        weights = np.zeros(len(tel_combinations))
        # Get offset angle for model selection
        offset_angle = compute_angle_separation(
            event.dl2.geometry["HillasReconstructor"].alt, 
            event.dl2.geometry["HillasReconstructor"].az, 
            self.array_pointing_direction.altitude,
            self.array_pointing_direction.azimuth
        )
        
        # Prepare feature arrays for batch prediction if possible
        features_list = []
        for tel_id in self.telescopes:
            features = self.get_features(event, tel_id)
            features_list.append(features)
        
        # Try batch prediction
        try:
            batch_features = np.vstack(features_list)
            batch_disps = self.predict(batch_features, offset_angle)
            #event.dl2.set_tel_estimate_disp(self.telescopes, batch_disps)
            for itel, tel_id in enumerate(self.telescopes):
                event.dl2.set_tel_disp(tel_id, batch_disps[itel])
            
            for i, (tel1_idx, tel2_idx) in enumerate([(self.telescopes.index(tel1), self.telescopes.index(tel2)) for tel1, tel2 in tel_combinations]):
                tel1, tel2 = tel_combinations[i]
                disp1 = batch_disps[tel1_idx]
                disp2 = batch_disps[tel2_idx]
                
                # Process the pair
                x_rec[i], y_rec[i], weights[i] = self._process_tel_pair(tel1, tel2, disp1, disp2)
                
        except:
            # Fall back to individual predictions
            for i, (tel1, tel2) in enumerate(tel_combinations):
                features1 = features_list[self.telescopes.index(tel1)]
                features2 = features_list[self.telescopes.index(tel2)]
                
                disp1 = self.predict(features1, offset_angle)
                disp2 = self.predict(features2, offset_angle)
                
                # Process the pair
                x_rec[i], y_rec[i], weights[i] = self._process_tel_pair(tel1, tel2, disp1, disp2)
        
        # Calculate final position and uncertainties
        fov_x = np.average(x_rec, weights=weights)
        fov_y = np.average(y_rec, weights=weights)
        rec_az, rec_alt = self.convert_to_sky(fov_x, fov_y)
        sigma_x = np.sqrt(np.average(np.square(x_rec - fov_x)))
        sigma_y = np.sqrt(np.average(np.square(y_rec - fov_y)))
        
        # Set geometry properties
        self.geometry.is_valid = True
        self.geometry.az = rec_az
        self.geometry.alt = rec_alt
        self.geometry.alt_uncertainty = sigma_x
        self.geometry.az_uncertainty = sigma_y
        self.geometry.set_telescopes(self.telescopes)
        self.geometry.direction_error = np.deg2rad(compute_angle_separation(
            rec_alt, rec_az, event.simulation.shower.alt, event.simulation.shower.az
        ))
        
        event.dl2.add_geometry(self.name, self.geometry)
    
    def _process_tel_pair(self, tel1, tel2, disp1, disp2):
        """Process a telescope pair to find the best intersection point"""
        # Get Hillas parameters for both telescopes
        hillas1_x = self.hillas_dicts[tel1].x
        hillas1_y = self.hillas_dicts[tel1].y
        hillas2_x = self.hillas_dicts[tel2].x
        hillas2_y = self.hillas_dicts[tel2].y
        
        # Calculate all four possible combinations of source positions
        pos1_plus = (hillas1_x + disp1 * np.cos(self.hillas_dicts[tel1].psi), 
                     hillas1_y + disp1 * np.sin(self.hillas_dicts[tel1].psi))
        pos1_minus = (hillas1_x - disp1 * np.cos(self.hillas_dicts[tel1].psi), 
                      hillas1_y - disp1 * np.sin(self.hillas_dicts[tel1].psi))
        pos2_plus = (hillas2_x + disp2 * np.cos(self.hillas_dicts[tel2].psi), 
                     hillas2_y + disp2 * np.sin(self.hillas_dicts[tel2].psi))
        pos2_minus = (hillas2_x - disp2 * np.cos(self.hillas_dicts[tel2].psi), 
                      hillas2_y - disp2 * np.sin(self.hillas_dicts[tel2].psi))
        
        # Calculate distances between all combinations
        d1 = np.sqrt((pos1_plus[0] - pos2_plus[0])**2 + (pos1_plus[1] - pos2_plus[1])**2)
        d2 = np.sqrt((pos1_plus[0] - pos2_minus[0])**2 + (pos1_plus[1] - pos2_minus[1])**2)
        d3 = np.sqrt((pos1_minus[0] - pos2_plus[0])**2 + (pos1_minus[1] - pos2_plus[1])**2)
        d4 = np.sqrt((pos1_minus[0] - pos2_minus[0])**2 + (pos1_minus[1] - pos2_minus[1])**2)
        
        # Find the two closest positions
        distances = [d1, d2, d3, d4]
        positions = [(pos1_plus, pos2_plus), (pos1_plus, pos2_minus), 
                     (pos1_minus, pos2_plus), (pos1_minus, pos2_minus)]
        
        min_idx = np.argmin(distances)
        closest_positions = positions[min_idx]
        
        # Average the two closest positions using hillas intensity as weights
        weight1 = self.hillas_dicts[tel1].intensity
        weight2 = self.hillas_dicts[tel2].intensity
        total_weight = weight1 + weight2
        x_rec = (closest_positions[0][0] * weight1 + closest_positions[1][0] * weight2) / total_weight
        y_rec = (closest_positions[0][1] * weight1 + closest_positions[1][1] * weight2) / total_weight
        
        return x_rec, y_rec, weight1 * weight2 / total_weight

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
        
        # Get energy estimates
        rec_energy = list(event.dl2.energy.values())[0].estimate_energy
        tel_rec_energy = dl2_tel.estimate_energy
        
        # Collect all features in the required order
        feature_values = [
            impact_parameter,                # rec_impact_parameter
            rec_energy,                      # rec_energy
            tel_rec_energy,                  # tel_rec_energy
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
            concentration.concentration_pixel, # concentration_pixel
            morphology.n_pixels,             # morphology_num_pixels
            morphology.n_islands,            # morphology_num_islands
            intensity.intensity_max,         # intensity_max
            intensity.intensity_mean,        # intensity_mean
            intensity.intensity_std,         # intensity_std
            self.n_tel                       # n_tel
        ]
        
        return np.array(feature_values)
