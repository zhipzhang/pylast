import json
import pickle
from ..helper import GeometryReconstructor as CGeometryReconstructor, compute_angle_separation
from ..lookuptable import Lookup2DTable,  SigmaLookupTableCollection
import numpy as np
import numba as nb
from iminuit import Minuit
import logging

logging.getLogger().setLevel(logging.INFO)
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
    return np.sum((distances**2)* (weights))/np.sum(weights)

class HillasWeightedReconstructor(CGeometryReconstructor):
    def __init__(self, subarray, config_str=None):
        super().__init__(subarray, config_str)
        self.name = "HillasWeightedReconstructor"
        if config_str is None:
            self.config = {}
        else:
            self.config = json.loads(config_str)
        if("beta_lookup_table_path" in self.config):
            self.beta_lookup_table = SigmaLookupTableCollection.load(self.config["beta_lookup_table_path"])
        else:
            raise ValueError("beta_lookup_table_path is not set")
        if("cog_lookup_table_path" in self.config):
            self.cog_lookup_table = SigmaLookupTableCollection.load(self.config["cog_lookup_table_path"])
        else:
            raise ValueError("cog_lookup_table_path is not set")
        
        # Configuration for iteration
        self.min_iterations = self.config.get("min_iterations", 2)
        self.max_iterations = self.config.get("max_iterations", 6)
        self.convergence_threshold = self.config.get("convergence_threshold", 1e-5)  # radians
    def __call__(self, event):
        # Make sure we have a dl2 event
        super().__call__(event)
        if len(self.hillas_dicts) < 2:
            self.geometry.is_valid = False
            event.dl2.add_geometry(self.name, self.geometry)
            return
        
        logging.debug(f'Handle event {event.event_id}')
        # Extract all hillas parameters in a single pass for better performance
        hillas_values = list(self.hillas_dicts.values())
        hillas_x = np.array([h.x for h in hillas_values])
        hillas_y = np.array([h.y for h in hillas_values])
        hillas_psi = np.array([h.psi for h in hillas_values])
        hillas_intensity = np.log10(np.array([h.intensity for h in hillas_values]))
        hillas_shape = np.array([h.width/h.length for h in hillas_values])
        
        # Get array pointing direction (set by super().__call__)
        array_pointing_az = self.array_pointing_direction.azimuth
        array_pointing_alt = self.array_pointing_direction.altitude
        
        # Get initial position from HillasReconstructor as starting point
        if event.dl2.geometry["HillasReconstructor"].is_valid:
            initial_x, initial_y = self.convert_to_fov(
                event.dl2.geometry["HillasReconstructor"].alt, 
                event.dl2.geometry["HillasReconstructor"].az
            )
            # Calculate initial offset: angular separation between HillasReconstructor result and array pointing
            hillas_reco_alt = event.dl2.geometry["HillasReconstructor"].alt
            hillas_reco_az = event.dl2.geometry["HillasReconstructor"].az
            offset = np.degrees(self.compute_angle_separation(
                array_pointing_az, array_pointing_alt,
                hillas_reco_az, hillas_reco_alt
            ))
        else:
            initial_x, initial_y = np.mean(hillas_x), np.mean(hillas_y)
            offset = 0.0  # No valid initial reconstruction, use 0
        logging.debug(f"Initial position: {initial_x}, {initial_y}")
        logging.debug(f"Initial offset: {offset}")

        # Initialize current position
        fov_x, fov_y = initial_x, initial_y
        
        # Iterative reconstruction with convergence check
        converged = False
        for iteration in range(self.max_iterations):
            # Lookup beta_err and cog_err based on offset, shape, and intensity
            # offset: angular separation between reconstructed direction and array pointing
            beta_err = self.beta_lookup_table(offset, hillas_shape, hillas_intensity)
            cog_err = self.cog_lookup_table(offset, hillas_shape, hillas_intensity)
            logging.debug(f"Offset: {offset}")
            logging.debug(f"Hillas shape: {hillas_shape}")
            logging.debug(f"Hillas intensity: {hillas_intensity}")
            logging.debug(f"Beta err: {beta_err}")
            logging.debug(f"Cog err: {cog_err}")
            # Apply filtering: only use telescopes with valid errors
            flag = (beta_err > 0) & (cog_err > 0) & np.isfinite(beta_err) & np.isfinite(cog_err)
            logging.debug(f"Flag: {flag}")
            if np.sum(flag) < 2:
                # Not enough valid telescopes
                self.geometry.is_valid = False
                event.dl2.add_geometry(self.name, self.geometry)
                return
            # if exactly 2 telescopes are valid, don't need to continue
            if np.sum(flag) == 2:
                logging.debug(f"Exactly 2 telescopes are valid, no need to continue")
                self.geometry.is_valid = True
                self.geometry.az = event.dl2.geometry["HillasReconstructor"].az
                self.geometry.alt = event.dl2.geometry["HillasReconstructor"].alt
                self.geometry.direction_error = event.dl2.geometry["HillasReconstructor"].direction_error
                self.geometry.alt_uncertainty = 0
                self.geometry.az_uncertainty = 0
                self.geometry.set_telescopes(self.telescopes)
                event.dl2.add_geometry(self.name, self.geometry)
                return
            
            # Filter arrays
            hillas_x_filtered = hillas_x[flag]
            hillas_y_filtered = hillas_y[flag]
            hillas_psi_filtered = hillas_psi[flag]
            beta_err_filtered = beta_err[flag]
            cog_err_filtered = cog_err[flag]
            logging.debug(f"Hillas x filtered: {hillas_x_filtered}")
            logging.debug(f"Hillas y filtered: {hillas_y_filtered}")
            logging.debug(f"Hillas psi filtered: {hillas_psi_filtered}")
            logging.debug(f"Beta err filtered: {beta_err_filtered}")
            logging.debug(f"Cog err filtered: {cog_err_filtered}")
            # Define the objective function for minimization
            def objective(params):
                x, y = params
                # Calculate disp for current position
                disp = np.sqrt(np.square(x - hillas_x_filtered) + np.square(y - hillas_y_filtered))
                # Calculate weights based on disp and errors
                weights = 1.0 / (np.power(disp * beta_err_filtered, 2) + np.power(cog_err_filtered, 2))
                return compute_distance(x, y, hillas_x_filtered, hillas_y_filtered, hillas_psi_filtered, weights)
            logging.debug(f"Objective: {objective((fov_x, fov_y))}")
            # Set up Minuit for minimization
            logging.debug(f"Setting up Minuit for minimization, current position: {fov_x}, {fov_y}")
            minuit = Minuit(objective, (fov_x, fov_y))
            minuit.errors = [np.radians(0.001), np.radians(0.001)]
            minuit.limits = [(np.radians(-7), np.radians(7)), (np.radians(-7), np.radians(7))]
            minuit.tol = 1e-7
            minuit.migrad()  # Find minimum
            # Get new position
            new_fov_x, new_fov_y = minuit.values
            logging.debug(f"New position: {new_fov_x}, {new_fov_y}")
            # Check convergence
            position_change = np.sqrt(np.square(new_fov_x - fov_x) + np.square(new_fov_y - fov_y))
            logging.debug(f"Position change: {position_change}")
            # Update position
            fov_x, fov_y = new_fov_x, new_fov_y
            
            # Calculate estimate_uncertainty at the optimized position
            # Calculate disp at optimized position
            disp_optimized = np.sqrt(np.square(fov_x - hillas_x_filtered) + np.square(fov_y - hillas_y_filtered))
            
            # Calculate individual measurement uncertainties
            # sigma_i = sqrt((disp * beta_err)^2 + cog_err^2)
            individual_uncertainties = np.sqrt(
                np.power(disp_optimized * beta_err_filtered, 2) + np.power(cog_err_filtered, 2)
            )
            
            # Calculate estimate_uncertainty using error propagation formula
            # 1/sigma^2 = sum(1/sigma_i^2)
            sum_inv_var = np.sum(1.0 / np.power(individual_uncertainties, 2))
            estimate_uncertainty = 1.0 / np.sqrt(sum_inv_var)
            
            # Update offset for next iteration based on new reconstructed position
            if iteration < self.max_iterations - 1:  # Don't need to recalculate on last iteration
                rec_az_temp, rec_alt_temp = self.convert_to_sky(fov_x, fov_y)
                offset = np.degrees(self.compute_angle_separation(
                    array_pointing_az, array_pointing_alt,
                    rec_az_temp, rec_alt_temp
                ))
                logging.debug(f"New offset: {offset}")
            
            # Check if we've done minimum iterations and converged
            if iteration >= self.min_iterations - 1:
                if position_change < self.convergence_threshold:
                    converged = True
                    break
        
        # Convert to sky coordinates
        rec_az, rec_alt = self.convert_to_sky(fov_x, fov_y)
        
        # Use estimate_uncertainty calculated from error propagation
        # This combines all telescope measurements: 1/sigma^2 = sum(1/sigma_i^2)
        if minuit.valid and converged:
            alt_uncertainty = estimate_uncertainty
        else:
            alt_uncertainty = np.nan
        
        # Set geometry properties
        self.geometry.is_valid = minuit.valid and converged
        self.geometry.az = rec_az
        self.geometry.alt = rec_alt
        self.geometry.alt_uncertainty = alt_uncertainty
        self.geometry.az_uncertainty = alt_uncertainty
        self.geometry.set_telescopes(self.telescopes)
        
        # Calculate direction error if simulation data is available
        if hasattr(event, 'simulation') and hasattr(event.simulation, 'shower'):
            self.geometry.direction_error = compute_angle_separation(
                event.simulation.shower.alt, event.simulation.shower.az, rec_alt, rec_az)
        
        event.dl2.add_geometry(self.name, self.geometry)
