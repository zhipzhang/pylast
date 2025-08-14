#include "RootEventSource.hh"
#include "EventMonitor.hh"
#include "ImageParameters.hh"
#include "RootDataLevels.hh"
#include "SubarrayDescription.hh"
#include "TDirectory.h"
#include "spdlog/spdlog.h"
#include <cassert>
#include "TKey.h"
#include <iostream>
#include "TH1F.h"
#include "TH2F.h"
RootEventSource::RootEventSource(const std::string& filename, int64_t max_events, std::vector<int> subarray, bool load_subarray_from_env)
    : EventSource(filename, max_events, subarray),
      load_subarray_from_env(load_subarray_from_env)
{
    spdlog::debug("RootEventSource constructor");
    initialize();
    initialize_array_event();
    initialize_statistics();
}
const std::string ihep_url = "root://eos01.ihep.ac.cn:/";
void RootEventSource::open_file()
{
    if(input_filename.substr(0, 4) == "/eos")
    {
        input_filename = ihep_url + input_filename;
    }
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
        spdlog::warn("no subarray directory found");
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
        else {
            spdlog::warn("no subarray directory found, using empty subarray");
            subarray = SubarrayDescription();
            return;
        }
    }
    subarray = SubarrayDescription();
    auto tel_pos_tree = static_cast<TTree*>(subarray_dir->Get("tel_positions"));
    int tel_id;
    RVecD* tel_pos = nullptr;
    tel_pos_tree->SetBranchAddress("tel_id", &tel_id);
    tel_pos_tree->SetBranchAddress("position", &tel_pos);
    auto optics_tree = static_cast<TTree*>(subarray_dir->Get("optics"));
    config_helper.root_optics_description = RootOpticsDescription();
    config_helper.root_optics_description->initialize_read(optics_tree);
    auto camera_dir = subarray_dir->GetDirectory("camera/");
    auto camera_geometry_tree = static_cast<TTree*>(camera_dir->Get("geometry"));
    auto camera_readout_tree = static_cast<TTree*>(camera_dir->Get("readout"));
    config_helper.root_camera_geometry = RootCameraGeometry();
    config_helper.root_camera_geometry->initialize_read(camera_geometry_tree);
    config_helper.root_camera_readout = RootCameraReadout();
    config_helper.root_camera_readout->initialize_read(camera_readout_tree);
    assert(optics_tree->GetEntries() == camera_geometry_tree->GetEntries());
    assert(optics_tree->GetEntries() == camera_readout_tree->GetEntries());
    assert(tel_pos_tree->GetEntries() == optics_tree->GetEntries());
    for(int i = 0; i < optics_tree->GetEntries(); ++i)
    {
        std::array<double, 3> tel_position = {(*tel_pos)[0], (*tel_pos)[1], (*tel_pos)[2]};
        auto telescope_description = config_helper.get_telescope_description(i);
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
        spdlog::debug("no cfg directory found");
        return;
    }
    auto atmosphere_model_tree = static_cast<TTree*>(cfg_dir->Get("atmosphere_model"));
    if(!atmosphere_model_tree)
    {
        spdlog::debug("no atmosphere model tree found");
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
    spdlog::debug("atmosphere model tree entries: {}", atmosphere_model_tree->GetEntries());
    atmosphere_model->n_alt = alt_km->size();
    atmosphere_model->alt_km = Eigen::Map<Eigen::VectorXd>(alt_km->data(), alt_km->size());
    atmosphere_model->rho = Eigen::Map<Eigen::VectorXd>(rho->data(), rho->size());
    atmosphere_model->thick = Eigen::Map<Eigen::VectorXd>(thick->data(), thick->size());
    atmosphere_model->refidx_m1 = Eigen::Map<Eigen::VectorXd>(refidx_m1->data(), refidx_m1->size());
}
void RootEventSource::load_all_simulated_showers()
{
    spdlog::warn("Not implemented");
}

template<typename T>
void RootEventSource::initialize_dl2_trees(const std::string& subdir, std::unordered_map<std::string, std::optional<T>>& tree_map)
{
    TDirectory* dir = file->GetDirectory(("/events/dl2/" + subdir).c_str());
    if(!dir) {
        return;
    }
    
    TList* keys = dir->GetListOfKeys();
    for(int i = 0; i < keys->GetSize(); i++) {
        TKey* key = static_cast<TKey*>(keys->At(i));
        if(strcmp(key->GetClassName(), "TTree") == 0) {
            std::string tree_name = key->GetName();
            auto tree = static_cast<TTree*>(dir->Get(tree_name.c_str()));
            if(tree) {
                tree_map[tree_name] = T();
                tree_map[tree_name]->initialize_read(tree);
                spdlog::debug("Found {} tree: {}", subdir, tree_name);
            }
        }
    }
}

void RootEventSource::initialize_array_event()
{
    // Test What we have in the root file
    initialize_dir("/events/simulation", "shower", event_helper.root_simulation_shower);
    initialize_dir("/events", "event_index", event_helper.root_event_index);
    // Initialize R0 data level
    initialize_data_level("simulation", event_helper.root_simulation_camera);
    initialize_data_level("r0", event_helper.root_r0_camera);
    initialize_data_level("r1", event_helper.root_r1_camera);
    initialize_data_level("dl0", event_helper.root_dl0_camera);
    initialize_data_level("dl1", event_helper.root_dl1_camera);
    initialize_data_level("dl2", event_helper.root_dl2_camera);
    initialize_data_level("monitor", event_helper.root_tel_monitor);

    initialize_dir("/events", "pointing", event_helper.root_pointing);
    // Initialize DL2 trees using the template function
    initialize_dl2_trees("geometry", event_helper.root_dl2_rec_geometry_map);
    initialize_dl2_trees("energy", event_helper.root_dl2_rec_energy_map);
    initialize_dl2_trees("particle", event_helper.root_dl2_rec_particle_map);
    max_events = event_helper.root_event_index->index_tree->GetEntries();
    
}

template<typename T>
void RootEventSource::initialize_data_level(const std::string& level_name, std::optional<T>& data_level)
    {
        TDirectory* dir = file->GetDirectory(("/events/" + level_name).c_str());
        if(!dir)
        {
            spdlog::debug("no {} directory found", level_name);
            return;
        }
        
        auto tree = static_cast<TTree*>(dir->Get("tels"));
        if(!tree)
        {
            spdlog::debug("no {} tree found in {} directory", level_name, level_name);
            return;
        }
        
        data_level = T();
        data_level->initialize_read(tree);
    }

template<typename T>
void RootEventSource::initialize_dir(const std::string& subdir, const std::string& tree_name,  std::optional<T>& structure)
{
    TDirectory* dir = file->GetDirectory(subdir.c_str());
    if(!dir)
    {
        spdlog::debug("no {} directory found", subdir);
        return;
    }
    auto tree = static_cast<TTree*>(dir->Get(tree_name.c_str()));
    if(!tree)
    {
        spdlog::debug("no {} tree found in {} directory", tree_name, subdir);
        return;
    }

    structure = T();
    structure->initialize_read(tree);
};
ArrayEvent RootEventSource::get_event()
{
    if(is_finished())
    {
        spdlog::warn("No more events to read");
        return ArrayEvent();
    }
    return event_helper.get_event();
}

ArrayEvent RootEventSource::get_event(int index)
{
    if(index < 0 || index >= max_events)
    {
        throw std::out_of_range("Index out of range: " + std::to_string(index));
    }
    return event_helper.get_event(index);
}
bool RootEventSource::is_finished() 
{
    return event_helper.current_entry >= max_events;
}

void RootEventSource::initialize_statistics()
{
    TDirectory* statistics_dir = file->GetDirectory("/statistics/");
    if(!statistics_dir)
    {
        spdlog::debug("no statistics directory found");
        return;
    }
    // Get all histogram objects in the statistics directory
    TIter next(statistics_dir->GetListOfKeys());
    TKey* key;
    statistics = Statistics();
    
    while ((key = static_cast<TKey*>(next()))) {
        TObject* obj = key->ReadObj();
        std::string name = obj->GetTitle();
        
        // Handle 1D histograms
        if (obj->IsA() == TH1F::Class()) {
            TH1F* h1 = static_cast<TH1F*>(obj);
            int nbins = h1->GetNbinsX();
            float min = h1->GetXaxis()->GetXmin();
            float max = h1->GetXaxis()->GetXmax();
            
            auto hist = make_regular_histogram<float>(min, max, nbins);
            
            // Fill the histogram with bin contents
            for (int i = 1; i <= nbins; i++) {
                float bin_center = h1->GetBinCenter(i);
                float bin_content = h1->GetBinContent(i);
                if (bin_content > 0) {
                    hist.fill(bin_center, bin_content);
                }
            }
            
            statistics->add_histogram(name, hist);
        }
        // Handle 2D histograms
        else if (obj->IsA() == TH2F::Class()) {
            TH2F* h2 = static_cast<TH2F*>(obj);
            int nbins_x = h2->GetNbinsX();
            int nbins_y = h2->GetNbinsY();
            float min_x = h2->GetXaxis()->GetXmin();
            float max_x = h2->GetXaxis()->GetXmax();
            float min_y = h2->GetYaxis()->GetXmin();
            float max_y = h2->GetYaxis()->GetXmax();
            
            auto hist = make_regular_histogram_2d<float>(min_x, max_x, nbins_x, min_y, max_y, nbins_y);
            
            // Fill the histogram with bin contents
            for (int i = 1; i <= nbins_x; i++) {
                for (int j = 1; j <= nbins_y; j++) {
                    float bin_content = h2->GetBinContent(i, j);
                    if (bin_content > 0) {
                        float x_center = h2->GetXaxis()->GetBinCenter(i);
                        float y_center = h2->GetYaxis()->GetBinCenter(j);
                        hist.fill(x_center, y_center, bin_content);
                    }
                }
            }
            statistics->add_histogram(name, hist);
        }
    }
    
}