import json
import pickle
from ..helper import GeometryReconstructor as CGeometryReconstructor, compute_angle_separation
import numpy as np
import numba as nb
from iminuit import Minuit

@nb.njit
def compute_distance(source_x, source_y, hillas_x, hillas_y, hillas_psi, weights):
    """
    Compute the weighted sum of distances from a source point to all Hillas parameter lines.
    
    Args:
        source_x (float): X-coordinate of the source point in camera coordinates
        source_y (float): Y-coordinate of the source point in camera coordinates
        hillas_x (numpy.ndarray): Array of x-coordinates of Hillas centroids
        hillas_y (numpy.ndarray): Array of y-coordinates of Hillas centroids
        hillas_psi (numpy.ndarray): Array of Hillas orientation angles
        miss (numpy.ndarray): Array of weights for each distance calculation
        
    Returns:
        float: Weighted sum of perpendicular distances from the source point to all Hillas lines
    """
    # Normal vectors to the lines (perpendicular to psi direction)
    # Normal vectors to the major axis of the ellipse
    nx = np.sin(hillas_psi)
    ny = -np.cos(hillas_psi)
    
    # Calculate C in the line equation A*x + B*y + C = 0
    # where the line passes through the Hillas centroid with slope determined by psi
    c = -(nx * hillas_x + ny * hillas_y)
    
    # Calculate perpendicular distances from the source point to each line
    # This is |Ax + By + C| / sqrt(A² + B²), but since nx² + ny² = 1, we can simplify
    distances = np.abs(nx * source_x + ny * source_y + c)
    
    # Return weighted sum of all distances
    return np.sum((distances** 2) * (weights ** 2))/np.sum(weights ** 2)

class HillasWeightedReconstructor(CGeometryReconstructor):
    def __init__(self, subarray, config_str=None):
        super().__init__(subarray, config_str)
        self.name = "HillasWeightedReconstructor"
        if config_str is None:
            self.config = {}
        else:
            self.config = json.loads(config_str)
        #if("model_path" in self.config):
        #    with open(self.config["model_path"], "rb") as f:
        #        self.model = pickle.load(f)
        #else:
        #    raise ValueError("model_path is not set")
        #self.check_model()
    #def check_model(self):
    #    if self.model is None:
    #        raise ValueError("model is not set")
    #    else:
    #        self.miss_predictor = self.model["miss_predictor"]
    def __call__(self, event):
        # Make sure we have a dl2 event
        super().__call__(event)
        if(len(self.hillas_dicts) < 2):
            self.geometry.is_valid = False
            event.dl2.add_geometry(self.name, self.geometry)
            return
        hillas_x = np.array([hillas.x for hillas in self.hillas_dicts.values()])
        hillas_y = np.array([hillas.y for hillas in self.hillas_dicts.values()])
        hillas_psi = np.array([hillas.psi for hillas in self.hillas_dicts.values()])
        weights = 1/np.array([event.dl1.tels[tel_id].image_parameters.extra.miss for tel_id in self.hillas_dicts.keys()])
            
        initial_x, initial_y = np.mean(hillas_x), np.mean(hillas_y)
        
        # Iterate 3 times with different weighting schemes for disp
        fov_x, fov_y = initial_x, initial_y
        
        for i in range(1):
            # Define the objective function for minimization
            def objective(params):
                x, y = params
                return compute_distance(x, y, hillas_x, hillas_y, hillas_psi, weights)
            
            # Use previous result as initial guess after first iteration
            minuit = Minuit(objective, (fov_x, fov_y))
            minuit.errors = [np.radians(0.0001), np.radians(0.0001)]
            minuit.limits = [(np.radians(-7), np.radians(7)), (np.radians(-7), np.radians(7))]
            minuit.tol = 1e-7 
            # Set up Minuit for minimization
            minuit.migrad() # Find minimum
            
            # Update best fit parameters for next iteration
            fov_x, fov_y = minuit.values
        # Convert to sky coordinates
        rec_az, rec_alt = self.convert_to_sky(fov_x, fov_y)
        
        # Calculate uncertainties
        sigma_x = minuit.errors[0]
        sigma_y = minuit.errors[1]
        
        # Set geometry properties
        self.geometry.is_valid = True
        self.geometry.az = rec_az
        self.geometry.alt = rec_alt
        self.geometry.alt_uncertainty = sigma_y
        self.geometry.az_uncertainty = sigma_x
        self.geometry.set_telescopes(self.telescopes)
        
        # Calculate direction error if simulation data is available
        if hasattr(event, 'simulation') and hasattr(event.simulation, 'shower'):
            self.geometry.direction_error = compute_angle_separation(
                event.simulation.shower.alt, event.simulation.shower.az, rec_alt, rec_az)
        
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

        intensity = dl1_tel.image_parameters.intensity
        
        # Calculate area (length * width * pi)
        shape = hillas.length / hillas.width
        # Get impact parameter from DL2 if available
        impact_parameter = dl2_tel.impact.distance
        
        # Collect all features in the required order
        feature_values = [
            impact_parameter,
            np.log10(hillas.intensity),
            shape,                            # shape parameter
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
            concentration.concentration_pixel, # concentration_pixel
            morphology.n_islands,            # morphology_n_islands
            morphology.n_large_islands,      # morphology_n_large_islands
            morphology.n_medium_islands,     # morphology_n_medium_islands
            morphology.n_pixels,             # morphology_n_pixels
            intensity.intensity_max,         # intensity_max
            intensity.intensity_mean,        # intensity_mean
            intensity.intensity_std,         # intensity_std
        ]
        
        return np.array(feature_values)
