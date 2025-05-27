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
    spdlog::debug("Not implemented");
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
                tree_map[tree_name] = T(tree_name);
                tree_map[tree_name]->initialize(tree);
                spdlog::debug("Found {} tree: {}", subdir, tree_name);
            }
        }
    }
}

void RootEventSource::initialize_array_event()
{
    // Test What we have in the root file
    TDirectory* simulation_dir = file->GetDirectory("/events/simulation");
    if(!simulation_dir)
    {
        spdlog::warn("no simulation directory found");
    }
    else
    {
        auto sim_tree = static_cast<TTree*>(simulation_dir->Get("shower"));
        if(!sim_tree)
        {
            spdlog::debug("no shower tree found in simulation directory");
        }
        else
        {
            array_event.simulation_shower = RootSimulationShower();
            array_event.simulation_shower->initialize(sim_tree);
        }
    }

    initialize_event_index();
    // Initialize R0 data level
    initialize_data_level<RootSimulatedCamera>("simulation", array_event.simulation_camera);
    initialize_data_level<RootR0Event>("r0", array_event.r0);
    initialize_data_level<RootR1Event>("r1", array_event.r1);
    initialize_data_level<RootDL0Event>("dl0", array_event.dl0);
    initialize_data_level<RootDL1Event>("dl1", array_event.dl1);
    initialize_data_level<RootDL2Event>("dl2", array_event.dl2);
    initialize_data_level<RootMonitor>("monitor", array_event.monitor);
    
    // Pointing need special handling
    TDirectory* pointing_dir = file->GetDirectory("/events/");
    if(!pointing_dir)
    {
        spdlog::debug("no events directory found");
    }
    else
    {
        auto pointing_tree = static_cast<TTree*>(pointing_dir->Get("pointing"));
        if(!pointing_tree)
        {
            spdlog::debug("no pointing tree found");
        }
        else
        {
            array_event.pointing = RootPointing();
            array_event.pointing->initialize(pointing_tree);
        }
    }

    // Initialize DL2 trees using the template function
    initialize_dl2_trees("geometry", array_event.dl2_geometry_map);
    initialize_dl2_trees("energy", array_event.dl2_energy_map);
    initialize_dl2_trees("particle", array_event.dl2_particle_map);
    
    max_events = array_event.test_entries();
}

void RootEventSource::initialize_event_index()
{
    TDirectory* event_index_dir = file->GetDirectory("/events/");
    if(!event_index_dir)
    {
        spdlog::debug("no events directory found");
    }
    else
    {
        auto event_index_tree = static_cast<TTree*>(event_index_dir->Get("event_index"));
        if(!event_index_tree)
        {
            spdlog::debug("no event_index tree found");
        }
        else
        {
            array_event.event_index = RootEventIndex();
            array_event.event_index->initialize(event_index_tree);
        }
    }
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
        data_level->initialize(tree);
    }

ArrayEvent RootEventSource::get_event()
{
    if(is_finished())
    {
        return ArrayEvent();
    }
    ArrayEvent event;
    array_event.load_next_event();
    if(array_event.simulation_shower.has_value())
    {
        event.simulation = SimulatedEvent();
        event.simulation->shower = array_event.simulation_shower->shower;
        event.event_id = array_event.simulation_shower->event_id;
    }
    else
    {
        event.event_id = 0;
    }
    if(array_event.simulation_camera.has_value())
    {
        if(event.simulation.has_value())
        {
            for(auto ientry: array_event.sim_tel_entries)
            {
                array_event.simulation_camera->get_entry(ientry);
                int tel_id = array_event.simulation_camera->tel_id;
                int n_pixels = array_event.simulation_camera->true_image.size();
                event.simulation->add_simulated_image(tel_id, n_pixels, array_event.simulation_camera->true_image.data(), array_event.simulation_camera->true_impact_parameter);
            }
        }
    }
    if(array_event.r0.has_value())
    {
        event.r0 = R0Event();
        for(auto ientry: array_event.r0_tel_entries)
        {
            array_event.r0->get_entry(ientry);
            int tel_id = array_event.r0->tel_id;
            int n_pixels = array_event.r0->n_pixels;
            int n_samples = array_event.r0->n_samples;
            Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> high_gain_waveform = Eigen::Map<Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>>(array_event.r0->high_gain_waveform.data(), n_pixels, n_samples);
            Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor> low_gain_waveform = Eigen::Map<Eigen::Matrix<uint16_t, -1, -1, Eigen::RowMajor>>(array_event.r0->low_gain_waveform.data(), n_pixels, n_samples);
            R0Camera r0_camera(n_pixels, n_samples, high_gain_waveform, low_gain_waveform);
            event.r0->add_tel(tel_id, n_pixels, n_samples, std::move(high_gain_waveform), std::move(low_gain_waveform), nullptr, nullptr);
        }
    }
    if(array_event.r1.has_value())
    {
        event.r1 = R1Event();
        for(auto ientry: array_event.r1_tel_entries)
        {
            array_event.r1->get_entry(ientry);
            int tel_id = array_event.r1->tel_id;
            int n_pixels = array_event.r1->n_pixels;
            int n_samples = array_event.r1->n_samples;
            Eigen::Matrix<double, -1, -1, Eigen::RowMajor> waveform = Eigen::Map<Eigen::Matrix<double, -1, -1, Eigen::RowMajor>>(array_event.r1->waveform.data(), n_pixels, n_samples);
            Eigen::VectorXi gain_selection = Eigen::Map<Eigen::VectorXi>(array_event.r1->gain_selection.data(), n_pixels);
            event.r1->add_tel(tel_id, n_pixels, n_samples, std::move(waveform), std::move(gain_selection));
        }
    }
    if(array_event.dl0.has_value())
    {
        event.dl0 = DL0Event();
        for(auto ientry: array_event.dl0_tel_entries)
        {
            array_event.dl0->get_entry(ientry);
            int tel_id = array_event.dl0->tel_id;
            int n_pixels = array_event.dl0->n_pixels;
            Eigen::VectorXd image = Eigen::Map<Eigen::VectorXd>(array_event.dl0->image.data(), n_pixels);
            Eigen::VectorXd peak_time = Eigen::Map<Eigen::VectorXd>(array_event.dl0->peak_time.data(), n_pixels);
            event.dl0->add_tel(tel_id, std::move(image), std::move(peak_time));
        }
    }
    if(array_event.dl1.has_value())
    {
        event.dl1 = DL1Event();
        for(auto ientry: array_event.dl1_tel_entries)
        {
            array_event.dl1->get_entry(ientry);
            int tel_id = array_event.dl1->tel_id;
            Eigen::VectorXf image = Eigen::Map<Eigen::VectorXf>(array_event.dl1->image.data(), array_event.dl1->image.size());
            Eigen::VectorXf peak_time = Eigen::Map<Eigen::VectorXf>(array_event.dl1->peak_time.data(), array_event.dl1->peak_time.size());
            Eigen::Vector<bool, -1> mask = Eigen::Map<Eigen::Vector<bool, -1>>(array_event.dl1->mask.data(), array_event.dl1->mask.size());

            DL1Camera dl1_camera;
            dl1_camera.image = std::move(image);
            dl1_camera.peak_time = std::move(peak_time);
            dl1_camera.mask = std::move(mask);
            dl1_camera.image_parameters = array_event.dl1->params;
            event.dl1->add_tel(tel_id, std::move(dl1_camera));
        }
    }
    if(array_event.dl2.has_value())
    {
        event.dl2 = DL2Event();
        for(auto ientry: array_event.dl2_tel_entries)
        {
            array_event.dl2->get_entry(ientry);
            int tel_id = array_event.dl2->tel_id;
            event.dl2->add_tel_geometry(tel_id, array_event.dl2->distance, array_event.dl2->reconstructor_name);
            event.dl2->tels.at(tel_id).estimate_energy = array_event.dl2->estimate_energy;
            event.dl2->tels.at(tel_id).disp = array_event.dl2->estimate_disp;
        }
    }
    for(auto [name, geometry]: array_event.dl2_geometry_map)
    {
        if(geometry.has_value())
        {
            event.dl2->geometry[name] = geometry->geometry;
        }
    }
    for(auto [name, energy]: array_event.dl2_energy_map)
    {
        if(energy.has_value())
        {
            event.dl2->energy[name] = energy->energy;
        }
    }
    for(auto [name, particle]: array_event.dl2_particle_map)
    {
        if(particle.has_value())
        {
            event.dl2->particle[name] = particle->particle;
        }
    }
    if(array_event.monitor.has_value())
    {
        event.monitor = EventMonitor();
        for(auto ientry: array_event.monitor_tel_entries)
        {
            array_event.monitor->get_entry(ientry);
            int tel_id = array_event.monitor->tel_id;
            int n_channels = array_event.monitor->n_channels;
            int n_pixels = array_event.monitor->n_pixels;
            Eigen::Matrix<double, -1, -1, Eigen::RowMajor> dc_to_pe = Eigen::Map<Eigen::Matrix<double, -1, -1, Eigen::RowMajor>>(array_event.monitor->dc_to_pe.data(), n_channels, n_pixels);
            Eigen::Matrix<double, -1, -1, Eigen::RowMajor> pedestals = Eigen::Map<Eigen::Matrix<double, -1, -1, Eigen::RowMajor>>(array_event.monitor->pedestals.data(), n_channels, n_pixels);
            event.monitor->add_tel(tel_id, n_channels, n_pixels, std::move(pedestals), std::move(dc_to_pe));
        }
    }
    if(array_event.pointing.has_value())
    {
        event.pointing = Pointing();
        event.pointing->array_altitude = array_event.pointing->array_alt;
        event.pointing->array_azimuth = array_event.pointing->array_az;
        for(auto tel_id: array_event.pointing->tel_id)
        {
            double azimuth = array_event.pointing->tel_az[tel_id];
            double altitude = array_event.pointing->tel_alt[tel_id];
            event.pointing->add_tel(tel_id, azimuth, altitude);
        }
    }
    if(event.event_id == 0)
    {
        // try to get event_id from dl1
        if(array_event.dl1.has_value())
        {
            event.event_id = array_event.dl1->event_id;
        }
    }
    return event;
}

bool RootEventSource::is_finished() 
{
    return !array_event.has_event();
}

ArrayEvent RootEventSource::operator[](int index)
{
    if(index >= max_events)
    {
        throw std::runtime_error("index out of range");
    }
    array_event.get_event(index);
    return get_event();
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