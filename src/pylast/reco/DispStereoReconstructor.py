import json
import pickle
from ..helper import GeometryReconstructor as CGeometryReconstructor
import numpy as np
import os
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
            self.disp_predictor = self.disp_model["disp_predictor"]
    def __call__(self, event):
        # Make sure we have a dl2 event
        super().__call__(event)
        if(len(self.hillas_dicts) < 2 or list(event.dl2.energy.values())[0].energy_valid is False):
            self.geometry.is_valid = False
            event.dl2.add_geometry(self.name, self.geometry)
            return
        # Create arrays to store all possible telescope combinations
        # Using itertools.combinations to generate all possible pairs of telescopes
        import itertools
        self.n_tel = len(self.telescopes)
        # Generate all possible pairs of telescopes
        tel_combinations = list(itertools.combinations(self.telescopes, 2))
        x_rec = np.zeros(len(tel_combinations))
        y_rec = np.zeros(len(tel_combinations))

        for i, (tel1, tel2) in enumerate(tel_combinations):
            features1 = self.get_features(event, tel1)
            features2 = self.get_features(event, tel2)
            disp1 = self.disp_predictor.predict(features1)
            disp2 = self.disp_predictor.predict(features2)
            
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
            x_rec[i] = (closest_positions[0][0] * weight1 + closest_positions[1][0] * weight2) / total_weight
            y_rec[i] = (closest_positions[0][1] * weight1 + closest_positions[1][1] * weight2) / total_weight
            
        fov_x = np.average(x_rec)
        fov_y = np.average(y_rec)
        rec_az, rec_alt = self.convert_to_sky(fov_x, fov_y)
        sigma_x = np.sqrt(np.average(np.square(x_rec - fov_x)))
        sigma_y = np.sqrt(np.average(np.square(y_rec - fov_y)))
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
        

        rec_energy = list(event.dl2.energy.values())[0].estimate_energy
        # Extract Hillas parameters
        hillas = dl1_tel.image_parameters.hillas
        
        # Extract leakage parameters
        leakage = dl1_tel.image_parameters.leakage
        
        # Extract concentration parameters
        concentration = dl1_tel.image_parameters.concentration
        
        # Extract morphology parameters
        morphology = dl1_tel.image_parameters.morphology
        intensity = dl1_tel.image_parameters.intensity
        impact_parameter = dl2_tel.impact.distance
        tel_rec_energy = dl2_tel.estimate_energy
        # Collect all features in the required order
        feature_values = [
            impact_parameter,                # rec_impact_parameter
            rec_energy,                      # rec_energy
            tel_rec_energy,                  # tel_rec_energy
            hillas.length,                   # hillas_length
            hillas.width,                    # hillas_width
            hillas.x,                        # hillas_x
            hillas.y,                        # hillas_y
            hillas.phi,                      # hillas_phi
            hillas.psi,                      # hillas_psi
            hillas.r,                        # hillas_r
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
            morphology.n_small_islands,      # morphology_num_small_islands
            morphology.n_medium_islands,     # morphology_num_medium_islands
            morphology.n_large_islands,      # morphology_num_large_islands
            intensity.intensity_max,                   # intensity_max
            intensity.intensity_mean,                  # intensity_mean
            intensity.intensity_std,                   # intensity_std
            self.n_tel              # n_tel
        ]
        
        return np.array([feature_values])
