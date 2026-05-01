#include "TTree.h"
#include "ImageParameters.hh"
#include "SimulatedShower.hh"
#include "ROOT/RVec.hxx"
struct TelescopeData
{
    int64_t event_id;
    int tel_id;
    ImageParameters params;
    ImageParameters fake_params;
    double rec_impact_parameter;
    double true_impact_parameter;
    double true_alt;
    double true_az;
    double true_energy;
    double xmax;
    double rec_alt;
    double rec_az;
    double rec_energy;
    double tel_rec_energy;
    double tel_rec_hadroness;
    double hillas_hmax;
    int n_tel;
    double average_intensity;
    double tel_rec_energy_std;
    double tel_rec_disp;
};
struct EventData
{
    int64_t event_id;
    int hillas_n_tels;
    SimulatedShower shower;
    double hillas_rec_alt;
    double hillas_rec_az;
    double hillas_rec_core_x;
    double hillas_rec_core_y;
    double hillas_direction_error;
    double hillas_direction_sigma;
    double rec_energy;
    double rec_energy_std;
    double flow_rec_energy;
    double hadroness;
    double mrsl;
    double mrsw;
    double weighted_summed_rec_alt;
    double weighted_sum_rec_az;
    double weighted_sum_direction_error;
    double weighted_sum_direction_sigma;

    double gw;
    double disp_rec_alt;
    double disp_rec_az;
    double disp_direction_error;
    double disp_direction_sigma;

    double test_rec_alt;
    double test_rec_az;
    double test_direction_error;
    double test_rec_core_x;
    double test_rec_core_y;
    double hillas_hmax;
    double pointing_alt;
    double pointing_az;
};
void initialize_telescope_tree(TTree *tree,  TelescopeData &data);

void initialize_telescope_tree(TTree* tree,  TelescopeData& data)
{
    tree->Branch("event_id", &data.event_id);
    tree->Branch("tel_id", &data.tel_id);
    tree->Branch("rec_impact_parameter", &data.rec_impact_parameter);
    tree->Branch("true_impact_parameter", &data.true_impact_parameter);
    tree->Branch("n_tel", &data.n_tel);
    tree->Branch("true_alt", &data.true_alt);
    tree->Branch("true_az", &data.true_az);
    tree->Branch("true_energy", &data.true_energy);
    tree->Branch("rec_alt", &data.rec_alt);
    tree->Branch("rec_az", &data.rec_az);
    tree->Branch("rec_energy", &data.rec_energy);
    tree->Branch("tel_rec_energy", &data.tel_rec_energy);
    tree->Branch("tel_rec_disp", &data.tel_rec_disp);
    tree->Branch("tel_rec_energy_std", &data.tel_rec_energy_std);
    tree->Branch("xmax", &data.xmax);
    // Image parameters
    tree->Branch("hillas_length", &data.params.hillas.length);
    tree->Branch("hillas_width", &data.params.hillas.width);
    tree->Branch("hillas_x", &data.params.hillas.x);
    tree->Branch("hillas_y", &data.params.hillas.y);
    tree->Branch("hillas_phi", &data.params.hillas.phi);
    tree->Branch("hillas_psi", &data.params.hillas.psi);
    tree->Branch("hillas_r", &data.params.hillas.r);
    tree->Branch("hillas_skewness", &data.params.hillas.skewness);
    tree->Branch("hillas_kurtosis", &data.params.hillas.kurtosis);
    tree->Branch("hillas_intensity", &data.params.hillas.intensity);
    tree->Branch("hillas_hmax", &data.hillas_hmax);

    tree->Branch("fake_hillas_length", &data.fake_params.hillas.length);
    tree->Branch("fake_hillas_width", &data.fake_params.hillas.width);
    tree->Branch("fake_hillas_x", &data.fake_params.hillas.x);
    tree->Branch("fake_hillas_y", &data.fake_params.hillas.y);
    tree->Branch("fake_hillas_phi", &data.fake_params.hillas.phi);
    tree->Branch("fake_hillas_psi", &data.fake_params.hillas.psi);
    tree->Branch("fake_hillas_r", &data.fake_params.hillas.r);
    tree->Branch("fake_hillas_skewness", &data.fake_params.hillas.skewness);
    tree->Branch("fake_hillas_kurtosis", &data.fake_params.hillas.kurtosis);
    tree->Branch("fake_hillas_intensity", &data.fake_params.hillas.intensity);
    tree->Branch("fake_hillas_scale_ratio", &data.fake_params.hillas.scale_ratio);
    tree->Branch("fake_leakage_pixels_width_1", &data.fake_params.leakage.pixels_width_1);
    tree->Branch("fake_leakage_pixels_width_2", &data.fake_params.leakage.pixels_width_2);
    tree->Branch("fake_leakage_intensity_width_1", &data.fake_params.leakage.intensity_width_1);
    tree->Branch("fake_leakage_intensity_width_2", &data.fake_params.leakage.intensity_width_2);
    tree->Branch("fake_concentration_cog", &data.fake_params.concentration.concentration_cog);
    tree->Branch("fake_concentration_core", &data.fake_params.concentration.concentration_core);
    tree->Branch("fake_concentration_pixel", &data.fake_params.concentration.concentration_pixel);
    tree->Branch("fake_concentration_frac2", &data.fake_params.concentration.concentration_frac2);
    tree->Branch("fake_morphology_num_pixels", &data.fake_params.morphology.n_pixels);
    tree->Branch("fake_morphology_num_islands", &data.fake_params.morphology.n_islands);
    tree->Branch("fake_morphology_num_small_islands", &data.fake_params.morphology.n_small_islands);
    tree->Branch("fake_morphology_num_medium_islands", &data.fake_params.morphology.n_medium_islands);
    tree->Branch("fake_morphology_num_large_islands", &data.fake_params.morphology.n_large_islands);
    tree->Branch("fake_intensity_max", &data.fake_params.intensity.intensity_max);
    tree->Branch("fake_intensity_mean", &data.fake_params.intensity.intensity_mean);
    tree->Branch("fake_intensity_std", &data.fake_params.intensity.intensity_std);
    // Leakage parameters
    tree->Branch("leakage_pixels_width_1", &data.params.leakage.pixels_width_1);
    tree->Branch("leakage_pixels_width_2", &data.params.leakage.pixels_width_2);
    tree->Branch("leakage_intensity_width_1", &data.params.leakage.intensity_width_1);
    tree->Branch("leakage_intensity_width_2", &data.params.leakage.intensity_width_2);
    
    // Concentration parameters
    tree->Branch("concentration_cog", &data.params.concentration.concentration_cog);
    tree->Branch("concentration_core", &data.params.concentration.concentration_core);
    tree->Branch("concentration_pixel", &data.params.concentration.concentration_pixel);
    
    // Morphology parameters
    tree->Branch("morphology_num_pixels", &data.params.morphology.n_pixels);
    tree->Branch("morphology_num_islands", &data.params.morphology.n_islands);
    tree->Branch("morphology_num_small_islands", &data.params.morphology.n_small_islands);
    tree->Branch("morphology_num_medium_islands", &data.params.morphology.n_medium_islands);
    tree->Branch("morphology_num_large_islands", &data.params.morphology.n_large_islands);
    
    tree->Branch("intensity_max", &data.params.intensity.intensity_max);
    tree->Branch("intensity_mean", &data.params.intensity.intensity_mean);
    tree->Branch("intensity_std", &data.params.intensity.intensity_std);
    tree->Branch("average_intensity", &data.average_intensity);
    // Extra parameters
    tree->Branch("extra_miss", &data.params.extra.miss);
    tree->Branch("extra_disp", &data.params.extra.disp);
    tree->Branch("extra_theta", &data.params.extra.theta);
    tree->Branch("extra_true_psi", &data.params.extra.true_psi);
    tree->Branch("extra_cog_err", &data.params.extra.cog_err);
    tree->Branch("extra_beta_err", &data.params.extra.beta_err);

    tree->Branch("fake_extra_miss", &data.fake_params.extra.miss);
    tree->Branch("fake_extra_disp", &data.fake_params.extra.disp);
    tree->Branch("fake_extra_theta", &data.fake_params.extra.theta);
    tree->Branch("fake_extra_true_psi", &data.fake_params.extra.true_psi);
    tree->Branch("fake_extra_cog_err", &data.fake_params.extra.cog_err);
    tree->Branch("fake_extra_beta_err", &data.fake_params.extra.beta_err);

    tree->Branch("two_gaussian_fit_converged", &data.fake_params.two_gaussian_fit.converged);
    tree->Branch("two_gaussian_fit_status", &data.fake_params.two_gaussian_fit.status);
    tree->Branch("two_gaussian_fit_amplitude", &data.fake_params.two_gaussian_fit.amplitude);
    tree->Branch("two_gaussian_fit_mean_x", &data.fake_params.two_gaussian_fit.mean_x);
    tree->Branch("two_gaussian_fit_mean_y", &data.fake_params.two_gaussian_fit.mean_y);
    tree->Branch("two_gaussian_fit_length", &data.fake_params.two_gaussian_fit.length);
    tree->Branch("two_gaussian_fit_width", &data.fake_params.two_gaussian_fit.width);
    tree->Branch("two_gaussian_fit_psi", &data.fake_params.two_gaussian_fit.psi);
    tree->Branch("two_gaussian_fit_beta_err", &data.fake_params.two_gaussian_fit.beta_err);
    tree->Branch("two_gaussian_fit_miss", &data.fake_params.two_gaussian_fit.miss);
    tree->Branch("two_gaussian_fit_use_gaussian_fit", &data.fake_params.two_gaussian_fit.use_gaussian_fit);
    tree->Branch("two_gaussian_fit_chi2", &data.fake_params.two_gaussian_fit.chi2);
    tree->Branch("two_gaussian_fit_fit_size", &data.fake_params.two_gaussian_fit.fit_size);
    tree->Branch("two_gaussian_fit_cog_err", &data.fake_params.two_gaussian_fit.cog_err);
    tree->Branch("two_gaussian_fit_disp", &data.fake_params.two_gaussian_fit.disp);
    tree->Branch("tel_rec_hadroness", &data.tel_rec_hadroness);
}

void initialize_event_tree(TTree* tree,  EventData& data)
{
    tree->Branch("event_id", &data.event_id);
    tree->Branch("hillas_n_tels", &data.hillas_n_tels);
    tree->Branch("hillas_direction_error", &data.hillas_direction_error);
    tree->Branch("hillas_direction_sigma", &data.hillas_direction_sigma);
    tree->Branch("hillas_rec_alt", &data.hillas_rec_alt);
    tree->Branch("hillas_rec_az", &data.hillas_rec_az);
    tree->Branch("hillas_rec_core_x", &data.hillas_rec_core_x);
    tree->Branch("hillas_rec_core_y", &data.hillas_rec_core_y);
    tree->Branch("hillas_hmax", &data.hillas_hmax);
    tree->Branch("energy", &data.shower.energy);
    tree->Branch("alt", &data.shower.alt);
    tree->Branch("az", &data.shower.az);
    tree->Branch("core_x", &data.shower.core_x);
    tree->Branch("core_y", &data.shower.core_y);
    tree->Branch("shower_primary_id", &data.shower.shower_primary_id);
    tree->Branch("h_first_int", &data.shower.h_first_int);
    tree->Branch("x_max", &data.shower.x_max);
    tree->Branch("h_max", &data.shower.h_max);
    tree->Branch("starting_grammage", &data.shower.starting_grammage);
    tree->Branch("rec_energy", &data.rec_energy);
    tree->Branch("rec_energy_std", &data.rec_energy_std);
    tree->Branch("flow_rec_energy", &data.flow_rec_energy);
    tree->Branch("mrsl", &data.mrsl);
    tree->Branch("mrsw", &data.mrsw);
    tree->Branch("weighted_summed_rec_alt", &data.weighted_summed_rec_alt);
    tree->Branch("weighted_sum_rec_az", &data.weighted_sum_rec_az);
    tree->Branch("weighted_sum_direction_error", &data.weighted_sum_direction_error);
    tree->Branch("weighted_sum_direction_sigma", &data.weighted_sum_direction_sigma);
    tree->Branch("disp_rec_alt", &data.disp_rec_alt);
    tree->Branch("disp_rec_az", &data.disp_rec_az);
    tree->Branch("disp_direction_error", &data.disp_direction_error);
    tree->Branch("disp_direction_sigma", &data.disp_direction_sigma);
    tree->Branch("hadroness", &data.hadroness);
    tree->Branch("pointing_alt", &data.pointing_alt);
    tree->Branch("pointing_az", &data.pointing_az);
    tree->Branch("test_rec_alt", &data.test_rec_alt);
    tree->Branch("test_rec_az", &data.test_rec_az);
    tree->Branch("test_direction_error", &data.test_direction_error);
    tree->Branch("test_rec_core_x", &data.test_rec_core_x);
    tree->Branch("test_rec_core_y", &data.test_rec_core_y);
    tree->Branch("gw", &data.gw);
}