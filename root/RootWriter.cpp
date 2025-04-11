#include "RootWriter.hh"
#include "SimulatedShower.hh"
#include "SimulationConfiguration.hh"
#include "spdlog/spdlog.h"
#include "ROOT/RVec.hxx"
#include "DataWriterFactory.hh"
#include "TH1F.h"
#include "TH2F.h"

REGISTER_WRITER(root, [](EventSource& source, const std::string& filename) { return std::make_unique<RootWriter>(source, filename); });
RootWriter::RootWriter(EventSource& source, const std::string& filename):
    FileWriter(source, filename)
{
    spdlog::debug("RootWriter constructor");
}


void RootWriter::open(bool overwrite)
{
    if(overwrite)
    {
        file = std::unique_ptr<TFile>(TFile::Open(filename.c_str(), "RECREATE"));
    }
    else
    {
        file = std::unique_ptr<TFile>(TFile::Open(filename.c_str(), "NEW"));
        if(!file)
        {
            throw std::runtime_error("file already exists: " + filename);
        }
    }
}

void RootWriter::close()
{
    for(auto& [name, tree] : trees)
    {
        if(directories.find(name) == directories.end())
        {
            throw std::runtime_error("directory not found: " + name);
        }
        directories[name]->cd();
        if(build_index[name])
        {
            int ret = tree->BuildIndex("event_id", "tel_id");
            if(ret < 0)
            {
                throw std::runtime_error("failed to build index for tree: " + name);
            }
            spdlog::info("building index for tree: {}", name);
        }
        tree->Write();
    }
    file->Write();
    spdlog::info("writing file: {}", filename);
}

void RootWriter::write_atmosphere_model()
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }
    if(!source.atmosphere_model)
    {
        throw std::runtime_error("atmosphere model not set");
    }
    using namespace ROOT;
    RVecD alt_km(source.atmosphere_model->alt_km.data(), source.atmosphere_model->alt_km.size());
    RVecD rho(source.atmosphere_model->rho.data(), source.atmosphere_model->rho.size());
    RVecD thick(source.atmosphere_model->thick.data(), source.atmosphere_model->thick.size());
    RVecD refidx_m1(source.atmosphere_model->refidx_m1.data(), source.atmosphere_model->refidx_m1.size());
    TDirectory* dir = file->mkdir("cfg/");
    if(!dir)
    {
        dir = file->GetDirectory("cfg/");
        if(!dir)
        {
            throw std::runtime_error("failed to create directory: cfg/");
        }
    }
    dir->cd();
    std::unique_ptr<TTree> tree = std::make_unique<TTree>("atmosphere_model", "atmosphere model");
    tree->Branch("alt_km", &alt_km);
    tree->Branch("rho", &rho);
    tree->Branch("thick", &thick);
    tree->Branch("refidx_m1", &refidx_m1);
    tree->Fill();
    tree->Write();
}

/*
For SubarrayDescription, we need to write the following information:
- telescope_description(camera_geometry, camera_readout(most of time is not needed for writing) optics_description)
- tel_positions
- reference_position

Try to write subarray description to a tree and each entry is a telescope description.
*/
void RootWriter::write_subarray()
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }
    if(!source.subarray)
    {
        throw std::runtime_error("subarray not set");
    }
    using namespace ROOT;
    auto ordered_tel_ids = source.subarray->get_ordered_telescope_ids();
    RVecD reference_position(source.subarray->reference_position.data(), source.subarray->reference_position.size());
    
    // Create a directory for subarray data
    TDirectory* dir = file->mkdir("subarray/");
    if(!dir)
    {
        dir = file->GetDirectory("subarray/");
        if(!dir)
        {
            throw std::runtime_error("failed to create directory: subarray/");
        }
    }
    
    // Write reference position
    dir->cd();
    std::unique_ptr<TTree> ref_tree = std::make_unique<TTree>("reference_position", "Array reference position");
    ref_tree->Branch("position", &reference_position);
    ref_tree->Fill();
    ref_tree->Write();
    
    // Create tree for telescope positions
    std::unique_ptr<TTree> tel_pos_tree = std::make_unique<TTree>("tel_positions", "Telescope positions");
    int tel_id = 0;
    RVecD tel_position;
    tel_pos_tree->Branch("tel_id", &tel_id);
    tel_pos_tree->Branch("position", &tel_position);
    
    // Write telescope positions
    for(const auto& id : ordered_tel_ids)
    {
        tel_id = id;
        tel_position = RVecD(source.subarray->tel_positions[id].data(), source.subarray->tel_positions[id].size());
        tel_pos_tree->Fill();
    }
    tel_pos_tree->Write();
    
    // Considering the optics description is quite simple, we can just write it to a tree
    std::unique_ptr<TTree> optics_tree = std::make_unique<TTree>("optics", "Telescope optics information");
    double mirror_area = 0.0;
    double equivalent_focal_length = 0.0;
    double effective_focal_length = 0.0;
    std::string mirror_name;
    int num_mirrors = 0;
    optics_tree->Branch("tel_id", &tel_id);
    optics_tree->Branch("mirror_name", &mirror_name);
    optics_tree->Branch("mirror_area", &mirror_area);
    optics_tree->Branch("equivalent_focal_length", &equivalent_focal_length);
    optics_tree->Branch("effective_focal_length", &effective_focal_length);
    optics_tree->Branch("num_mirrors", &num_mirrors);
    // Write optics information for each telescope
    for(const auto& id : ordered_tel_ids)
    {
        if(!source.subarray->tels.count(id))
            continue;
            
        const auto& optics = source.subarray->tels.at(id).optics_description;
        tel_id = id;
        mirror_name = optics.optics_name;
        mirror_area = optics.mirror_area;
        equivalent_focal_length = optics.equivalent_focal_length;
        effective_focal_length = optics.effective_focal_length;
        num_mirrors = optics.num_mirrors;
        optics_tree->Fill();
    }
    optics_tree->Write();
    
    auto camera_directory = get_or_create_directory("subarray/camera");
    camera_directory->cd();
    // Create tree for camera geometry
    std::unique_ptr<TTree> camera_geometry_tree = std::make_unique<TTree>("geometry", "Camera geometry information");
    RVecD pix_x;
    RVecD pix_y;
    RVecD pix_area;
    RVecI pix_type;
    camera_geometry_tree->Branch("tel_id", &tel_id);
    camera_geometry_tree->Branch("pix_x", &pix_x);
    camera_geometry_tree->Branch("pix_y", &pix_y);
    camera_geometry_tree->Branch("pix_area", &pix_area);
    camera_geometry_tree->Branch("pix_type", &pix_type);
    
    // Write camera geometry for each telescope
    std::unique_ptr<TTree> camera_readout_tree = std::make_unique<TTree>("readout", "Telescope camera readout information");
    std::string camera_name;
    double sampling_rate;
    int n_channels;
    int n_pixels;
    int n_samples;
    RVecD reference_pulse_shape;
    int reference_pulse_shape_length;
    double reference_pulse_sample_width;
    camera_readout_tree->Branch("tel_id", &tel_id);
    camera_readout_tree->Branch("camera_name", &camera_name);
    camera_readout_tree->Branch("n_samples", &n_samples);
    camera_readout_tree->Branch("sampling_rate", &sampling_rate);
    camera_readout_tree->Branch("n_channels", &n_channels);
    camera_readout_tree->Branch("n_pixels", &n_pixels);
    camera_readout_tree->Branch("reference_pulse_shape", &reference_pulse_shape);
    camera_readout_tree->Branch("reference_pulse_shape_length", &reference_pulse_shape_length);
    camera_readout_tree->Branch("reference_pulse_sample_width", &reference_pulse_sample_width);
    
    for(const auto& id : ordered_tel_ids)
    {
        if(!source.subarray->tels.count(id))
            continue;
            
        auto& camera_geom = source.subarray->tels.at(id).camera_description.camera_geometry;
        auto& camera_readout = source.subarray->tels.at(id).camera_description.camera_readout;
        tel_id = id;
        
        // Convert Eigen vectors to RVecD
        pix_x = std::move(RVecD(camera_geom.pix_x.data(), camera_geom.pix_x.size()));
        pix_y = std::move(RVecD(camera_geom.pix_y.data(), camera_geom.pix_y.size()));
        pix_area = std::move(RVecD(camera_geom.pix_area.data(), camera_geom.pix_area.size()));
        pix_type = std::move(RVecI(camera_geom.pix_type.data(), camera_geom.pix_type.size()));
        camera_geometry_tree->Fill();

        camera_name = camera_readout.camera_name;
        sampling_rate = camera_readout.sampling_rate;
        n_channels = camera_readout.n_channels;
        n_pixels = camera_readout.n_pixels;
        n_samples = camera_readout.n_samples;
        reference_pulse_shape_length = camera_readout.reference_pulse_shape.cols();
        reference_pulse_shape = std::move(RVecD(camera_readout.reference_pulse_shape.data(), n_channels * reference_pulse_shape_length));
        reference_pulse_sample_width = camera_readout.reference_pulse_sample_width;
        camera_readout_tree->Fill();
    }
    camera_geometry_tree->Write();
    camera_readout_tree->Write();
    

}

// Helper method to create or get a directory
TDirectory* RootWriter::get_or_create_directory(const std::string& path)
{
    if(!file)
    {
        throw std::runtime_error("File not open");
    }

    // Check if directory already exists
    TDirectory* dir = file->GetDirectory(path.c_str());
    if(dir)
    {
        return dir;
    }
    
    // Create directory path
    size_t pos = 0;
    std::string current_path;
    TDirectory* current_dir = file.get();
    
    while(pos < path.size())
    {
        size_t next_pos = path.find('/', pos);
        if(next_pos == std::string::npos)
        {
            next_pos = path.size();
        }
        
        if(next_pos > pos)
        {
            std::string dir_name = path.substr(pos, next_pos - pos);
            if(!dir_name.empty())
            {
                current_path += (current_path.empty() ? "" : "/") + dir_name;
                TDirectory* next_dir = file->GetDirectory(current_path.c_str());
                if(!next_dir)
                {
                    current_dir = current_dir->mkdir(dir_name.c_str());
                    if(!current_dir)
                    {
                        throw std::runtime_error("Failed to create directory: " + current_path);
                    }
                }
                else
                {
                    current_dir = next_dir;
                }
            }
        }
        
        pos = next_pos + 1;
    }
    
    return current_dir;
}
void RootWriter::write_simulation_config()
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }
    if(!source.simulation_config.has_value())
    {
        return;
    }
    TDirectory* dir = get_or_create_directory("cfg/");
    dir->cd();
    std::unique_ptr<TTree> tree = std::make_unique<TTree>("simulation_config", "Simulation configuration");
    SimulationConfiguration config;
    initialize_simulation_config_branches(*tree, config);
    config = source.simulation_config.value();
    tree->Fill();
    tree->Write();
}


void RootWriter::write_event_index(const ArrayEvent& event)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }
    
    // Check for any available data level to index
    if(!event.r0.has_value() && 
       !event.r1.has_value() && 
       !event.dl0.has_value() && 
       !event.dl1.has_value() && 
       !event.dl2.has_value())
    {
        return;
    }
    auto index_tree = get_tree("event_index");
    if(!index_tree)
    {
        TDirectory* dir = get_or_create_directory("/events/");
        dir->cd();
        TTree* tree = new TTree("event_index", "Event index");
        array_event.event_index = RootEventIndex();
        tree->Branch("event_id", &array_event.event_index->event_id);
        tree->Branch("telescopes", &array_event.event_index->telescopes);
        trees["event_index"] = tree;
        directories["event_index"] = dir;
        index_tree = tree;
    }
    array_event.event_index->telescopes.clear();
    array_event.event_index->event_id = event.event_id;

    // Get telescopes from the first available data level
    if(event.r0.has_value()) {
        for(const auto& tel_id : event.r0->get_ordered_tels()) {
            array_event.event_index->telescopes.push_back(tel_id);
        }
    } else if(event.r1.has_value()) {
        for(const auto& tel_id : event.r1->get_ordered_tels()) {
            array_event.event_index->telescopes.push_back(tel_id);
        }
    } else if(event.dl0.has_value()) {
        for(const auto& tel_id : event.dl0->get_ordered_tels()) {
            array_event.event_index->telescopes.push_back(tel_id);
        }
    } else if(event.dl1.has_value()) {
        for(const auto& tel_id : event.dl1->get_ordered_tels()) {
            array_event.event_index->telescopes.push_back(tel_id);
        }
    } else if(event.dl2.has_value()) {
        for(const auto& [tel_id, _] : event.dl2->tels) {
                array_event.event_index->telescopes.push_back(tel_id);
        }
    }
    index_tree->Fill();
    
}
void RootWriter::write_simulation_shower(const ArrayEvent& event)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }

    if(!event.simulation.has_value())
    {
        // Nothing to write
        return;
    }
    
    // Get directory
    
    // Get simulation data
    
    // Create simulation tree
    auto sim_tree = get_tree("shower");
    if(!sim_tree)
    {
        array_event.simulation = RootSimulationShower();
        TDirectory* dir = get_or_create_directory("/events/simulation");
        dir->cd();
        sim_tree = array_event.simulation->initialize();
        directories["shower"] = dir;
        trees["shower"] = sim_tree;
    }
    auto& root_shower = array_event.simulation.value();
    const auto& sim = event.simulation.value();
    root_shower.event_id = event.event_id;
    root_shower.shower = sim.shower;
    sim_tree->Fill();
}

void RootWriter::write_r0(const ArrayEvent& event)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }

    if(!event.r0.has_value())
    {
        // Nothing to write
        return;
    }
    TTree* r0_tree = get_tree("r0");
    if(!r0_tree)
    {
        spdlog::debug("initialize r0");
        initialize_data_level<RootR0Event>("r0", array_event.r0);
        r0_tree = get_tree("r0");
    }
    auto& root_r0 = array_event.r0.value();
    const auto& r0 = event.r0.value();
    root_r0.event_id = event.event_id;
    for(const auto& [tel_id, camera] : r0.get_tels())
    {
        root_r0.tel_id = tel_id;
        root_r0.n_pixels = camera->waveform[0].rows();
        root_r0.n_samples = camera->waveform[0].cols();
        root_r0.low_gain_waveform = std::move(RVec<uint16_t>(camera->waveform[0].data(), root_r0.n_pixels * root_r0.n_samples));
        root_r0.high_gain_waveform = std::move(RVec<uint16_t>(camera->waveform[1].data(), root_r0.n_pixels * root_r0.n_samples));
        r0_tree->Fill();
    }
}

void RootWriter::write_r1(const ArrayEvent& event)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }

    if(!event.r1.has_value())
    {
        // Nothing to write
        return;
    }
    
    auto r1_tree = get_tree("r1");
    if(!r1_tree)
    {
        spdlog::debug("initialize r1");
        initialize_data_level<RootR1Event>("r1", array_event.r1);
        r1_tree = get_tree("r1");
    }
    auto& root_r1 = array_event.r1.value();
    const auto& r1 = event.r1.value();
    root_r1.event_id = event.event_id;
    for(const auto& [tel_id, camera] : r1.tels)
    {
        root_r1.tel_id = tel_id;
        root_r1.n_pixels = camera->waveform.rows();
        root_r1.n_samples = camera->waveform.cols();
        root_r1.waveform = std::move(RVecD(camera->waveform.data(), root_r1.n_pixels * root_r1.n_samples));
        root_r1.gain_selection = std::move(RVecI(camera->gain_selection.data(), camera->gain_selection.size()));
        r1_tree->Fill();
    }
}

void RootWriter::write_dl0(const ArrayEvent& event)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }

    if(!event.dl0.has_value())
    {
        // Nothing to write
        return;
    }
    
    auto dl0_tree = get_tree("dl0");
    if(!dl0_tree)
    {
        spdlog::debug("initialize dl0");
        initialize_data_level<RootDL0Event>("dl0", array_event.dl0);
        dl0_tree = get_tree("dl0");
    }
    auto& root_dl0 = array_event.dl0.value();
    const auto& dl0 = event.dl0.value();
    root_dl0.event_id = event.event_id;
    for(const auto& [tel_id, camera] : dl0.tels)
    {
        root_dl0.tel_id = tel_id;
        root_dl0.n_pixels = camera->image.size();
        root_dl0.image = std::move(RVecD(camera->image.data(), root_dl0.n_pixels));
        root_dl0.peak_time = std::move(RVecD(camera->peak_time.data(), root_dl0.n_pixels));
        dl0_tree->Fill();
    }
}

void RootWriter::write_dl1(const ArrayEvent& event, bool write_image)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }

    if(!event.dl1.has_value())
    {
        // Nothing to write
        return;
    }
    
    // Get trees or create if not exist
    auto dl1_tree = get_tree("dl1");
    if(!dl1_tree)
    {
        spdlog::debug("initialize dl1");
        initialize_data_level<RootDL1Event>("dl1", array_event.dl1);
        dl1_tree = get_tree("dl1");
        if(write_image)
        {
            dl1_tree->Branch("image", &array_event.dl1->image, 256000, 0);
            dl1_tree->Branch("peak_time", &array_event.dl1->peak_time, 256000, 0);
            dl1_tree->Branch("mask", &array_event.dl1->mask, 256000, 0);
        }
    }
    
    const auto& dl1 = event.dl1.value();
    auto& root_dl1 = array_event.dl1.value();
    root_dl1.event_id = event.event_id;
    for(const auto& [tid, camera] : dl1.tels)
    {
        root_dl1.tel_id = tid;
        if(write_image)
        {
            root_dl1.n_pixels = camera->image.size();
            root_dl1.image = std::move(RVecF(camera->image.data(), root_dl1.n_pixels));
            root_dl1.peak_time = std::move(RVecF(camera->peak_time.data(), root_dl1.n_pixels));
            root_dl1.mask = std::move(RVec<bool>(camera->mask.data(), root_dl1.n_pixels));
        }
        root_dl1.params.hillas = camera->image_parameters.hillas;
        root_dl1.params.leakage = camera->image_parameters.leakage;
        root_dl1.params.concentration = camera->image_parameters.concentration;
        root_dl1.params.morphology = camera->image_parameters.morphology;
        root_dl1.params.intensity = camera->image_parameters.intensity;
        if(camera->image_parameters.extra.has_value())
        {
            root_dl1.miss = camera->image_parameters.extra->miss;
            root_dl1.disp = camera->image_parameters.extra->disp;
        }
        dl1_tree->Fill();
    }
}

void RootWriter::write_dl2(const ArrayEvent& event)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }

    if(!event.dl2.has_value())
    {
        // Nothing to write
        return;
    }
    
    // Get DL2 directory
    
    const auto& dl2 = event.dl2.value();
    for(const auto& [name, geom] : dl2.geometry)
    {
        auto geom_tree = get_tree(name);
        if(!geom_tree)
        {
            TDirectory* dir = get_or_create_directory("/events/dl2/geometry");
            dir->cd();
            array_event.dl2_geometry_map[name] = RootDL2Geometry(name);
            geom_tree = array_event.dl2_geometry_map[name]->initialize();
            trees[name] = geom_tree;
            directories[name] = dir;
        }
        auto& root_geom = array_event.dl2_geometry_map[name].value();
        root_geom.event_id = event.event_id;
        root_geom.reconstructor_name = name;
        root_geom.geometry = geom;
        geom_tree->Fill();
    }
    for(const auto& [name, energy] : dl2.energy)
    {
        auto energy_tree = get_tree(name);
        if(!energy_tree)
        {
            TDirectory* dir = get_or_create_directory("/events/dl2/energy");
            dir->cd();
            array_event.dl2_energy_map[name] = RootDL2Energy(name);
            energy_tree = array_event.dl2_energy_map[name]->initialize();
            trees[name] = energy_tree;
            directories[name] = dir;
        }
        auto& root_energy = array_event.dl2_energy_map[name].value();
        root_energy.event_id = event.event_id;
        root_energy.reconstructor_name = name;
        root_energy.energy = energy;
        energy_tree->Fill();
    }
    auto dl2_tree = get_tree("dl2");
    if(!dl2_tree)
    {
        spdlog::debug("initialize dl2");
        initialize_data_level<RootDL2Event>("dl2", array_event.dl2);
        dl2_tree = get_tree("dl2");
    }
    auto& root_dl2 = array_event.dl2.value();
    root_dl2.event_id = event.event_id;
    for(const auto& [tid, dl2_tel] : dl2.tels)
    {
        root_dl2.reconstructor_name.clear();
        root_dl2.distance.clear();
        root_dl2.distance_error.clear();
        root_dl2.tel_id = tid;
        root_dl2.estimate_energy = dl2_tel.estimate_energy;
        for(const auto& [name, impact] : dl2_tel.impact_parameters)
        {
            root_dl2.reconstructor_name.push_back(name);
            root_dl2.distance.push_back(impact.distance);
            root_dl2.distance_error.push_back(impact.distance_error);
        }
        dl2_tree->Fill();
    }
}

void RootWriter::write_monitor(const ArrayEvent& event)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }

    if(!event.monitor.has_value())
    {
        // Nothing to write
        return;
    }
    
    auto monitor_tree = get_tree("monitor");
    if(!monitor_tree)
    {
        spdlog::debug("initialize monitor");
        initialize_data_level<RootMonitor>("monitor", array_event.monitor);
        monitor_tree = get_tree("monitor");
    }
    
    const auto& monitor = event.monitor.value();
    auto& root_monitor = array_event.monitor.value();
    root_monitor.event_id = event.event_id;
    for(const auto& [tid, tel_monitor] : monitor.tels)
    {
        root_monitor.tel_id = tid;
        root_monitor.n_channels = tel_monitor->n_channels;
        root_monitor.n_pixels = tel_monitor->n_pixels;
        root_monitor.dc_to_pe = std::move(RVecD(tel_monitor->dc_to_pe.data(), root_monitor.n_channels * root_monitor.n_pixels));
        root_monitor.pedestals = std::move(RVecD(tel_monitor->pedestal_per_sample.data(), root_monitor.n_channels * root_monitor.n_pixels));
        monitor_tree->Fill();
    }
}

void RootWriter::write_pointing(const ArrayEvent& event)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }

    if(!event.pointing.has_value())
    {
        // Nothing to write
        return;
    }
    
    // Get pointing directory
    auto pointing_tree = get_tree("pointing");
    if(!pointing_tree)
    {
        array_event.pointing = RootPointing();
        TDirectory* dir = get_or_create_directory("/events/");
        dir->cd();
        pointing_tree = array_event.pointing->initialize();
        trees["pointing"] = pointing_tree;
        directories["pointing"] = dir;
    }
    const auto& pointing = event.pointing.value();
    auto& root_pointing = array_event.pointing.value();
    // Variables for identification
    root_pointing.event_id = event.event_id;
    root_pointing.array_alt = pointing.array_altitude;
    root_pointing.array_az = pointing.array_azimuth;
    for(const auto& [tid, point] : pointing.tels)
    {
        root_pointing.tel_id.push_back(tid);
        root_pointing.tel_az.push_back(point->azimuth);
        root_pointing.tel_alt.push_back(point->altitude);
    }
    pointing_tree->Fill();
    root_pointing.clear();
}

void RootWriter::write_event(const ArrayEvent& event)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }
    
    // Write each component that exists in the event
    // Typically this would check a configuration flag for each part
    
    // Create events directory
    get_or_create_directory("events");
    
    if(event.simulation.has_value())
    {
        write_simulation_shower(event);
    }
    write_event_index(event);
    if(event.r0.has_value())
    {
        write_r0(event);
    }
    
    if(event.r1.has_value())
    {
        write_r1(event);
    }
    
    if(event.dl0.has_value())
    {
        write_dl0(event);
    }
    
    if(event.dl1.has_value())
    {
        write_dl1(event);
    }
    
    if(event.dl2.has_value())
    {
        write_dl2(event);
    }
    
    if(event.monitor.has_value())
    {
        write_monitor(event);
    }
    
    if(event.pointing.has_value())
    {
        write_pointing(event);
    }
}


void RootWriter::initialize_simulation_config_branches(TTree& tree, SimulationConfiguration& config)
{
    tree.Branch("run_number", &config.run_number);
    tree.Branch("corsika_version", &config.corsika_version);
    tree.Branch("simtel_version", &config.simtel_version);
    tree.Branch("energy_range_min", &config.energy_range_min);
    tree.Branch("energy_range_max", &config.energy_range_max);
    tree.Branch("prod_site_B_total", &config.prod_site_B_total);
    tree.Branch("prod_site_B_declination", &config.prod_site_B_declination);
    tree.Branch("prod_site_B_inclination", &config.prod_site_B_inclination);
    tree.Branch("prod_site_alt", &config.prod_site_alt);
    tree.Branch("spectral_index", &config.spectral_index);
    tree.Branch("shower_prog_start", &config.shower_prog_start);
    tree.Branch("shower_prog_id", &config.shower_prog_id);
    tree.Branch("detector_prog_start", &config.detector_prog_start);
    tree.Branch("detector_prog_id", &config.detector_prog_id);
    tree.Branch("n_showers", &config.n_showers);
    tree.Branch("shower_reuse", &config.shower_reuse);
    tree.Branch("max_alt", &config.max_alt);
    tree.Branch("min_alt", &config.min_alt);
    tree.Branch("max_az", &config.max_az);
    tree.Branch("min_az", &config.min_az);
    tree.Branch("diffuse", &config.diffuse);
    tree.Branch("max_viewcone_radius", &config.max_viewcone_radius);
    tree.Branch("min_viewcone_radius", &config.min_viewcone_radius);
    tree.Branch("max_scatter_range", &config.max_scatter_range);
    tree.Branch("min_scatter_range", &config.min_scatter_range);
    tree.Branch("core_pos_mode", &config.core_pos_mode);
    tree.Branch("atmosphere", &config.atmosphere);
    tree.Branch("corsika_iact_options", &config.corsika_iact_options);
    tree.Branch("corsika_low_E_model", &config.corsika_low_E_model);
    tree.Branch("corsika_high_E_model", &config.corsika_high_E_model);
    tree.Branch("corsika_bunchsize", &config.corsika_bunchsize);
    tree.Branch("corsika_wlen_min", &config.corsika_wlen_min);
    tree.Branch("corsika_wlen_max", &config.corsika_wlen_max);
    tree.Branch("corsika_low_E_detail", &config.corsika_low_E_detail);
    tree.Branch("corsika_high_E_detail", &config.corsika_high_E_detail);
}


TTree* RootWriter::get_tree(const std::string& tree_name)
{
    if(trees.count(tree_name))
    {
        return trees[tree_name];
    }
    return nullptr;
}

template<typename T>
void RootWriter::initialize_data_level(const std::string& level_name, std::optional<T>& data_level)
{
    TDirectory* dir = get_or_create_directory("events/" + level_name);
    if(!dir)
    {
        throw std::runtime_error("failed to create directory: events/" + level_name);
    }
    dir->cd();
    data_level = T();
    auto tree = data_level->initialize();
    trees[level_name] = tree;
    directories[level_name] = dir;
    build_index[level_name] = true;
}
void RootWriter::write_statistics(const Statistics& statistics)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }
    TDirectory* dir = get_or_create_directory("statistics");
    dir->cd();
    // In ROOT Format, we just convert the histogram to TH1D Or TH2D
    int ihist = 0;
    for(const auto& [name, hist] : statistics.histograms)
    {
        if(hist->get_dimension() == 1)
        {
            auto h1d = dynamic_cast<Histogram1D<float>*>(hist.get());
            auto new_hist = new TH1F(("h" + std::to_string(ihist)).c_str(), name.c_str(), h1d->bins(), 
                                     h1d->get_low_edge(), h1d->get_high_edge());
            
            // Fill the histogram with bin contents
            for(int i = 0; i < h1d->bins(); i++)
            {
                new_hist->SetBinContent(i+1, h1d->get_bin_content(i));
            }
            
            new_hist->Write();
            ihist++;
        }
        else if(hist->get_dimension() == 2)
        {
            auto h2d = dynamic_cast<Histogram2D<float>*>(hist.get());
            auto new_hist = new TH2F(("h" + std::to_string(ihist)).c_str(), name.c_str(), 
                                     h2d->x_bins(), h2d->get_x_low_edge(), h2d->get_x_high_edge(),
                                     h2d->y_bins(), h2d->get_y_low_edge(), h2d->get_y_high_edge());
            
            // Fill the histogram with bin contents
            for(int i = 0; i < h2d->x_bins(); i++)
            {
                for(int j = 0; j < h2d->y_bins(); j++)
                {
                    new_hist->SetBinContent(i+1, j+1, h2d->operator()(i, j));
                }
            }
            new_hist->Write();
            ihist++;
        }
    }
}
