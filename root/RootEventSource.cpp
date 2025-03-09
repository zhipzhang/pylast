#include "RootEventSource.hh"
#include "SubarrayDescription.hh"
#include "spdlog/spdlog.h"
#include <cassert>


RootEventSource::RootEventSource(const std::string& filename, int64_t max_events, std::vector<int> subarray, bool load_subarray_from_env)
    : EventSource(filename, max_events, subarray)
{
    spdlog::debug("RootEventSource constructor");
    initialize();
}

void RootEventSource::open_file()
{
    file = std::unique_ptr<TFile>(TFile::Open(input_filename.c_str(), "READ"));
    if(!file)
    {
        throw std::runtime_error("file not found: " + input_filename);
    }
}


void RootEventSource::init_subarray()
{
    TDirectory* subarray_dir = file->GetDirectory("subarray/");
    if(!subarray_dir)
    {
        spdlog::debug("no subarray directory found");
        if(load_subarray_from_env)
        {
            std::string subarray_env = std::getenv("SUBARRAY");
            if(subarray_env.empty())
            {
                throw std::runtime_error("SUBARRAY environment variable not set");
            }
            auto subarray_file = std::unique_ptr<TFile>(TFile::Open(subarray_env.c_str(), "READ"));
            if(!subarray_file)
            {
                throw std::runtime_error("subarray file not found: " + subarray_env);
            }
            subarray_dir = subarray_file->GetDirectory("subarray/");
            if(!subarray_dir)
            {
                throw std::runtime_error("subarray directory not found in subarray file: " + subarray_env);
            }
        }
    }
    subarray = SubarrayDescription();
    auto tel_pos_tree = static_cast<TTree*>(subarray_dir->Get("tel_positions"));
    int tel_id;
    RVecD* tel_pos = nullptr;
    tel_pos_tree->SetBranchAddress("tel_id", &tel_id);
    tel_pos_tree->SetBranchAddress("position", &tel_pos);
    auto optics_tree = static_cast<TTree*>(subarray_dir->Get("optics"));
    auto camera_dir = subarray_dir->GetDirectory("camera/");
    auto camera_geometry_tree = static_cast<TTree*>(camera_dir->Get("geometry"));
    auto camera_readout_tree = static_cast<TTree*>(camera_dir->Get("readout"));
    assert(optics_tree->GetEntries() == camera_geometry_tree->GetEntries());
    assert(optics_tree->GetEntries() == camera_readout_tree->GetEntries());
    assert(tel_pos_tree->GetEntries() == optics_tree->GetEntries());
    std::string* mirror_name = nullptr;
    int num_mirrors;
    double mirror_area;
    double equivalent_focal_length;
    double effective_focal_length;
    optics_tree->SetBranchAddress("mirror_name", &mirror_name);
    optics_tree->SetBranchAddress("num_mirrors", &num_mirrors);
    optics_tree->SetBranchAddress("mirror_area", &mirror_area);
    optics_tree->SetBranchAddress("equivalent_focal_length", &equivalent_focal_length);
    optics_tree->SetBranchAddress("effective_focal_length", &effective_focal_length);

    RVecD* pix_x = nullptr;
    RVecD* pix_y = nullptr;
    RVecD* pix_area = nullptr;
    RVecI* pix_type = nullptr;
    camera_geometry_tree->SetBranchAddress("pix_x", &pix_x);
    camera_geometry_tree->SetBranchAddress("pix_y", &pix_y);
    camera_geometry_tree->SetBranchAddress("pix_area", &pix_area);
    camera_geometry_tree->SetBranchAddress("pix_type", &pix_type);

    std::string* camera_name = nullptr;
    double sampling_rate;
    int n_channels;
    int n_pixels;
    int n_samples;
    RVecD* reference_pulse_shape = nullptr;
    int reference_pulse_shape_length;
    double reference_pulse_sample_width;
    camera_readout_tree->SetBranchAddress("camera_name", &camera_name);
    camera_readout_tree->SetBranchAddress("n_samples", &n_samples);
    camera_readout_tree->SetBranchAddress("sampling_rate", &sampling_rate);
    camera_readout_tree->SetBranchAddress("n_channels", &n_channels);
    camera_readout_tree->SetBranchAddress("n_pixels", &n_pixels);
    camera_readout_tree->SetBranchAddress("reference_pulse_shape", &reference_pulse_shape);
    camera_readout_tree->SetBranchAddress("reference_pulse_shape_length", &reference_pulse_shape_length);
    camera_readout_tree->SetBranchAddress("reference_pulse_sample_width", &reference_pulse_sample_width);
    for(int i = 0; i < optics_tree->GetEntries(); ++i)
    {
        tel_pos_tree->GetEntry(i);
        optics_tree->GetEntry(i);
        camera_geometry_tree->GetEntry(i);
        camera_readout_tree->GetEntry(i);
        std::array<double, 3> tel_position = {(*tel_pos)[0], (*tel_pos)[1], (*tel_pos)[2]};
        OpticsDescription optics_description(*mirror_name, num_mirrors, mirror_area, equivalent_focal_length, effective_focal_length);
        CameraGeometry camera_geometry(*camera_name, n_pixels, pix_x->data(), pix_y->data(), pix_area->data(), pix_type->data(), 0.0);
        Eigen::MatrixXd reference_pulse_shape_matrix = Eigen::Map<Eigen::MatrixXd>(reference_pulse_shape->data(), n_channels, reference_pulse_shape_length);
        CameraReadout camera_readout(*camera_name, sampling_rate, reference_pulse_sample_width, n_channels, n_pixels, n_samples, reference_pulse_shape_matrix);
        auto telescope_description = TelescopeDescription(CameraDescription(*camera_name, std::move(camera_geometry), std::move(camera_readout)), (optics_description));
        subarray->add_telescope(tel_id, std::move(telescope_description), tel_position);
    }
}

void RootEventSource::init_metaparam()
{
    spdlog::debug("normally we don't need to set metaparam for root file");
}

void RootEventSource::init_simulation_config()
{
    spdlog::debug("normally we don't need to set simulation config for root file");
}

void RootEventSource::init_atmosphere_model()
{
    TDirectory* cfg_dir = file->GetDirectory("cfg/");
    if(!cfg_dir)
    {
        spdlog::error("no cfg directory found");
        return;
    }
    auto atmosphere_model_tree = static_cast<TTree*>(cfg_dir->Get("atmosphere_model"));
    if(!atmosphere_model_tree)
    {
        spdlog::error("no atmosphere model tree found");
        return;
    }
    atmosphere_model = TableAtmosphereModel();
    RVecD* alt_km = nullptr;
    RVecD* rho = nullptr;
    RVecD* thick = nullptr;
    RVecD* refidx_m1 = nullptr;
    atmosphere_model_tree->SetBranchAddress("alt_km", &alt_km);
    atmosphere_model_tree->SetBranchAddress("rho", &rho);
    atmosphere_model_tree->SetBranchAddress("thick", &thick);
    atmosphere_model_tree->SetBranchAddress("refidx_m1", &refidx_m1);
    for(int i = 0; i < atmosphere_model_tree->GetEntries(); ++i)
    {
        atmosphere_model_tree->GetEntry(i);
    }
    spdlog::error("atmosphere model tree entries: {}", atmosphere_model_tree->GetEntries());
    atmosphere_model->n_alt = alt_km->size();
    atmosphere_model->alt_km = Eigen::Map<Eigen::VectorXd>(alt_km->data(), alt_km->size());
    atmosphere_model->rho = Eigen::Map<Eigen::VectorXd>(rho->data(), rho->size());
    atmosphere_model->thick = Eigen::Map<Eigen::VectorXd>(thick->data(), thick->size());
    atmosphere_model->refidx_m1 = Eigen::Map<Eigen::VectorXd>(refidx_m1->data(), refidx_m1->size());
}
void RootEventSource::load_all_simulated_showers()
{
    spdlog::debug("normally we don't need to load simulated showers for root file");
}


