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

};
void initialize_telescope_tree(TTree *tree,  TelescopeData &data);

void initialize_telescope_tree(TTree* tree,  TelescopeData& data)
{
    tree->Branch("event_id", &data.event_id);
    tree->Branch("tel_id", &data.tel_id);
    tree->Branch("rec_impact_parameter", &data.rec_impact_parameter);
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
    
    // Extra parameters
    tree->Branch("extra_miss", &data.params.extra->miss);
    tree->Branch("extra_disp", &data.params.extra->disp);
}

void initialize_event_tree(TTree* tree,  EventData& data)
{
    tree->Branch("event_id", &data.event_id);
    tree->Branch("hillas_n_tels", &data.hillas_n_tels);

    tree->Branch("hillas_rec_alt", &data.hillas_rec_alt);
    tree->Branch("hillas_rec_az", &data.hillas_rec_az);
    tree->Branch("hillas_rec_core_x", &data.hillas_rec_core_x);
    tree->Branch("hillas_rec_core_y", &data.hillas_rec_core_y);
    tree->Branch("energy", &data.shower.energy);
    tree->Branch("alt", &data.shower.alt);
    tree->Branch("az", &data.shower.az);
    tree->Branch("core_x", &data.shower.core_x);
    tree->Branch("core_y", &data.shower.core_y);
    tree->Branch("shower_primary_id", &data.shower.shower_primary_id);
    tree->Branch("h_first_int", &data.shower.h_first_int);
    tree->Branch("x_max", &data.shower.x_max);
    tree->Branch("starting_grammage", &data.shower.starting_grammage);
}