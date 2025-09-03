#include "TTree.h"
#include "ImageParameters.hh"
#include "SimulatedShower.hh"
#include "ROOT/RVec.hxx"
struct TelescopeData
{
    int64_t event_id;
    int tel_id;
    ImageParameters params;
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
    double hillas_hmax;
    int n_tel;
    double average_intensity;
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
    double rec_energy;
    double hadroness;
    double disp_stereo_rec_alt;
    double disp_stereo_rec_az;
    double disp_direction_error;
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
}

void initialize_event_tree(TTree* tree,  EventData& data)
{
    tree->Branch("event_id", &data.event_id);
    tree->Branch("hillas_n_tels", &data.hillas_n_tels);
    tree->Branch("hillas_direction_error", &data.hillas_direction_error);
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
    tree->Branch("disp_stereo_rec_alt", &data.disp_stereo_rec_alt);
    tree->Branch("disp_stereo_rec_az", &data.disp_stereo_rec_az);
    tree->Branch("disp_direction_error", &data.disp_direction_error);
    tree->Branch("hadroness", &data.hadroness);
    tree->Branch("pointing_alt", &data.pointing_alt);
    tree->Branch("pointing_az", &data.pointing_az);
}