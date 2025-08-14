import json
import pickle

from ..helper import MLReconstructor as CMLReconstructor
from ..helper import ReconstructedParticle as CReconstructedParticle
import numpy as np
import os
from ..helper import compute_angle_separation

class MLParticleClassifier(CMLReconstructor):
    def __init__(self, config_str=None):
        super().__init__(config_str)
        self.name = "MLParticleClassifier"
        if config_str is None:
            self.config = {}
        else:
            self.config = json.loads(config_str)
        if("model_path" in self.config):
            with open(self.config["model_path"], "rb") as f:
                self.model = pickle.load(f)
        else:
            model_directory = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "model")
            particle_classifier_path = os.path.join(model_directory, "particle_classifier.pkl")
            with open(particle_classifier_path, "rb") as f:
                self.model = pickle.load(f)
        self.check_model()
        self.particle = CReconstructedParticle()
    def check_model(self):
        if self.model is None:
            raise ValueError("model is not set")
        else:
            self.particle_classifier = self.model["particle_classifier"]
    def __call__(self, event):
        super().__call__(event)
        if event.dl2.geometry["HillasReconstructor"].is_valid and event.dl2.energy["MLEnergyReconstructor"].energy_valid:
            features_list = []
            intensities = np.array([self.tel_rec_params[tel_id].hillas.intensity for tel_id in self.telescopes])
            self.average_intensity = np.mean(intensities)
            weights =  intensities
            for itel, tel_id in enumerate(self.telescopes):
                features = self.get_features(event, tel_id)
                features_list.append(features)
            batch_features = np.vstack(features_list)
            batch_predictions = self.particle_classifier.predict_proba(batch_features)
            for itel, tel_id in enumerate(self.telescopes):
                event.dl2.set_tel_estimate_hadroness(tel_id, batch_predictions[itel][1])
            self.particle.is_valid = True
            self.particle.hadroness = np.average(batch_predictions[:,1], weights=weights)
        else:
            self.particle.is_valid = False
            self.particle.hadroness = -99
        event.dl2.add_particle(self.name, self.particle)
    def get_features(self, event, tel_id):
        """
        Extract features from the event for the given telescope ID.
        """
        image_parameter = self.tel_rec_params[tel_id]
        dl2_tel = event.dl2.tels[tel_id]
        hillas = image_parameter.hillas
        leakage = image_parameter.leakage
        concentration = image_parameter.concentration
        morphology = image_parameter.morphology
        intensity = image_parameter.intensity

        impact_parameter = dl2_tel.impact.distance
        rec_energy = event.dl2.energy["MLEnergyReconstructor"].estimate_energy
        tel_rec_energy = dl2_tel.estimate_energy

        hmax = event.dl2.geometry["HillasReconstructor"].hmax;
        n_tel = len(self.telescopes)
        features = [
            impact_parameter,                # rec_impact_parameter
            hillas.length,                   # hillas_length
            hillas.width,                    # hillas_width
            hillas.r,
            hillas.psi,
            hillas.phi,
            hmax,
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
            morphology.n_small_islands,
            morphology.n_medium_islands,
            morphology.n_large_islands,
            intensity.intensity_max,         # intensity_max
            intensity.intensity_mean,        # intensity_mean
            intensity.intensity_std,         # intensity_std
            intensity.intensity_skewness,
            intensity.intensity_kurtosis,    # intensity_kurtosis
            self.average_intensity,
            n_tel,                           # n_tel
            rec_energy,                      # rec_energy
            tel_rec_energy,                  # tel_rec_energy
        ]
        return np.array(features)
        