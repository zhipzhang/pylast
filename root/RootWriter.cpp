#include "RootWriter.hh"
#include "RootDataLevels.hh"
#include "SimulatedShower.hh"
#include "SimulationConfiguration.hh"
#include "Statistics.hh"
#include "TDirectory.h"
#include "TH2.h"
#include "spdlog/spdlog.h"
#include "ROOT/RVec.hxx"
#include "DataWriterFactory.hh"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include <set>

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
        if(directories.contains(name))
        {
            directories[name]->cd();
        }
        else
        {
            gDirectory->cd("/");
        }
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
    write_statistics({}, true);
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
    config_helper.root_optics_description = RootOpticsDescription();
    config_helper.root_optics_description->initialize_write(optics_tree.get());
    auto& root_optics_description = config_helper.root_optics_description.value();

    for(const auto& id : ordered_tel_ids)
    {
        if(!source.subarray->tels.count(id))
            continue;
            
        const auto& optics = source.subarray->tels.at(id).optics_description;
        root_optics_description.tel_id = id;
        root_optics_description = optics;
        optics_tree->Fill();
    }
    optics_tree->Write();
    
    auto camera_directory = get_or_create_directory("subarray/camera");
    camera_directory->cd();
    // Create tree for camera geometry
    std::unique_ptr<TTree> camera_geometry_tree = std::make_unique<TTree>("geometry", "Camera geometry information");
    config_helper.root_camera_geometry = RootCameraGeometry();
    config_helper.root_camera_geometry->initialize_write(camera_geometry_tree.get());
    auto& root_camera_geometry = config_helper.root_camera_geometry.value();
    // Write camera geometry for each telescope
    std::unique_ptr<TTree> camera_readout_tree = std::make_unique<TTree>("readout", "Telescope camera readout information");
    config_helper.root_camera_readout = RootCameraReadout();
    config_helper.root_camera_readout->initialize_write(camera_readout_tree.get());
    auto& root_camera_readout = config_helper.root_camera_readout.value();
    for(const auto& id : ordered_tel_ids)
    {
        if(!source.subarray->tels.count(id))
            continue;
            
        const auto& camera_geom = source.subarray->tels.at(id).camera_description.camera_geometry;
        const auto& camera_readout = source.subarray->tels.at(id).camera_description.camera_readout;

        root_camera_readout.tel_id = id;
        root_camera_readout = camera_readout;
        root_camera_geometry.tel_id = id;
        root_camera_geometry = camera_geom;
        camera_geometry_tree->Fill();
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
void RootWriter::write_all_simulation_shower(const SimulatedShowerArray& shower_array)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }
    if(shower_array.size() == 0)
    {
        spdlog::warn("No simulated showers to write");
        return;
    }
    auto all_shower_tree = get_tree("all_shower");
    if(!all_shower_tree)
    {
        gDirectory->cd("/");
        spdlog::debug("initalize shower tree");
        auto tree = new TTree("shower", "All simulated showers");
        trees["all_shower"] = tree;
        all_shower_tree = tree;
        helper.root_simulation_shower = RootSimulationShower();
        helper.root_simulation_shower->initialize_write(tree);
    }
    for(int i = 0; i < shower_array.size(); ++i)
    {
        helper.root_simulation_shower->shower = shower_array[i];
        all_shower_tree->Fill();
    }
}
void RootWriter::write_simulation_config()
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }
    if(!source.simulation_config.has_value())
    {
        spdlog::warn("simulation configuration not set, skipping writing simulation configuration");
        return;
    }
    TDirectory* dir = get_or_create_directory("cfg/");
    dir->cd();
    std::unique_ptr<TTree> tree = std::make_unique<TTree>("simulation_config", "Simulation configuration");
    SimulationConfiguration config;
    initialize_simulation_config_branches(*tree, config);
    config = source.simulation_config.value();
    spdlog::warn("Writing simulation configuration: run_number = {}, corsika_high_E_detail = {}", 
             config.run_number, config.corsika_high_E_detail);
    tree->Fill();
    tree->Write();
}

void RootWriter::write_simulated_camera(const ArrayEvent& event, bool write_image)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }
    if(!event.simulation.has_value())
    {
        return;
    }
    auto sim_tree = get_tree("simulation");
    if(!sim_tree)
    {
        initialize_data_level("simulation", helper.root_simulation_camera);
        sim_tree = get_tree("simulation");
        if(write_image)
        {
            sim_tree->Branch("true_image", &helper.root_simulation_camera->true_image);
            sim_tree->Branch("fake_image", &helper.root_simulation_camera->fake_image);
            sim_tree->Branch("fake_image_mask", &helper.root_simulation_camera->fake_image_mask);
        }

    }
    auto& root_simulated_camera = helper.root_simulation_camera.value();
    const auto& sim = event.simulation.value();
    root_simulated_camera.event_id = event.event_id;
    for(const auto& [tel_id,camera] : event.simulation->tels)
    {
        root_simulated_camera.tel_id = tel_id;
        root_simulated_camera = std::move(*camera);
        sim_tree->Fill();
    }
    
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
        helper.root_event_index = RootEventIndex();
        auto tree = helper.root_event_index->initialize_write("event_index", "Event index for all data levels");
        trees["event_index"] = tree;
        directories["event_index"] = dir;
        index_tree = tree;
    }
    helper.root_event_index->telescopes.clear();
    helper.root_event_index->event_id = event.event_id;

    std::set<int> unique_telescopes;
    // Get telescopes from the first available data level
    if(event.simulation.has_value()) {
        for(const auto tel_id : event.simulation->get_ordered_tels()) {
            unique_telescopes.insert(tel_id);
        }
    }
    if(event.r0.has_value()) {
        for(const auto tel_id : event.r0->get_ordered_tels()) {
            unique_telescopes.insert(tel_id);
        }
    } 
    if(event.r1.has_value()) {
        for(const auto tel_id : event.r1->get_ordered_tels()) {
            unique_telescopes.insert(tel_id);
        }
    } 
    if(event.dl0.has_value()) {
        for(const auto tel_id : event.dl0->get_ordered_tels()) {
            unique_telescopes.insert(tel_id);
        }
    } 
    if(event.dl1.has_value()) {
        for(const auto tel_id : event.dl1->get_ordered_tels()) {
            unique_telescopes.insert(tel_id);
        }
    }
    if(event.dl2.has_value()) {
        for(const auto& [tel_id, _] : event.dl2->tels) {
            unique_telescopes.insert(tel_id);
        }
    }
    helper.root_event_index->telescopes = RVecI(unique_telescopes.begin(), unique_telescopes.end());
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
    auto shower_tree = get_tree("shower");
    if(!shower_tree)
    {
        helper.root_simulation_shower = RootSimulationShower();
        TDirectory* dir = get_or_create_directory("/events/simulation");
        dir->cd();
        auto tree = new TTree("shower", "Simulation shower data");
        shower_tree = tree;
        helper.root_simulation_shower->initialize_write(shower_tree);
        directories["shower"] = dir;
        trees["shower"] = shower_tree;
    }
    auto& root_simulation_shower = helper.root_simulation_shower.value();
    const auto& sim = event.simulation.value();
    root_simulation_shower.event_id = event.event_id;
    root_simulation_shower.shower = sim.shower;
    shower_tree->Fill();
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
        initialize_data_level("r0", helper.root_r0_camera);
        r0_tree = get_tree("r0");
    }
    const auto& r0 = event.r0.value();
    auto& root_r0_camera = helper.root_r0_camera.value();
    root_r0_camera.event_id = event.event_id;
    for(const auto& [tel_id, camera] : r0.get_tels())
    {
        root_r0_camera.tel_id = tel_id;
        root_r0_camera = std::move(*camera);
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
        initialize_data_level("r1", helper.root_r1_camera);
        r1_tree = get_tree("r1");
    }
    auto& root_r1_camera = helper.root_r1_camera.value();
    const auto& r1 = event.r1.value();
    root_r1_camera.event_id = event.event_id;
    for(const auto& [tel_id, camera] : r1.tels)
    {
        root_r1_camera.tel_id = tel_id;
        root_r1_camera = std::move(*camera);
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
        initialize_data_level("dl0", helper.root_dl0_camera);
        dl0_tree = get_tree("dl0");
    }
    auto& root_dl0_camera = helper.root_dl0_camera.value();
    const auto& dl0 = event.dl0.value();
    root_dl0_camera.event_id = event.event_id;
    for(const auto& [tel_id, camera] : dl0.tels)
    {
        root_dl0_camera.tel_id = tel_id;
        root_dl0_camera = std::move(*camera);
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
        initialize_data_level("dl1", helper.root_dl1_camera);
        dl1_tree = get_tree("dl1");
        if(write_image)
        {
            dl1_tree->Branch("image", &helper.root_dl1_camera->image);
            dl1_tree->Branch("peak_time", &helper.root_dl1_camera->peak_time);
            dl1_tree->Branch("mask", &helper.root_dl1_camera->mask);
        }
    }
    
    const auto& dl1 = event.dl1.value();
    auto& root_dl1_camera = helper.root_dl1_camera.value();
    root_dl1_camera.event_id = event.event_id;
    for(const auto& [tid, camera] : dl1.tels)
    {
        root_dl1_camera.tel_id = tid;
        if(write_image)
        {
            root_dl1_camera.image = std::move(RVecF(camera->image.data(), camera->image.size()));
            root_dl1_camera.peak_time = std::move(RVecF(camera->peak_time.data(), camera->peak_time.size()));
            root_dl1_camera.mask = std::move(RVecB(camera->mask.data(), camera->mask.size()));
        }
        root_dl1_camera.datalevels.image_parameters = camera->image_parameters;
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
            auto tree =  new TTree(name.c_str(), "Reconstructed geometry parameters");
            geom_tree = tree;
            helper.root_dl2_rec_geometry_map[name] = RootDL2RecGeometry();
            helper.root_dl2_rec_geometry_map[name]->initialize_write(geom_tree);
            trees[name] = geom_tree;
            directories[name] = dir;
        }
        auto& root_dl2_rec_geom = helper.root_dl2_rec_geometry_map[name].value();
        root_dl2_rec_geom.event_id = event.event_id;
        root_dl2_rec_geom = geom;
        geom_tree->Fill();
    }
    for(const auto& [name, energy] : dl2.energy)
    {
        auto energy_tree = get_tree(name);
        if(!energy_tree)
        {
            TDirectory* dir = get_or_create_directory("/events/dl2/energy");
            dir->cd();
            auto energy_tree = new TTree(name.c_str(), "Reconstructed energy parameters");
            helper.root_dl2_rec_energy_map[name] = RootDL2RecEnergy();
            helper.root_dl2_rec_energy_map[name]->initialize_write(energy_tree);
            trees[name] = energy_tree;
            directories[name] = dir;
        }
        auto& root_dl2_rec_energy = helper.root_dl2_rec_energy_map[name].value();
        root_dl2_rec_energy.event_id = event.event_id;
        root_dl2_rec_energy = energy;
        energy_tree->Fill();
    }
    for(const auto& [name, particle] : dl2.particle)
    {
        auto particle_tree = get_tree(name);
        if(!particle_tree)
        {
            TDirectory* dir = get_or_create_directory("/events/dl2/particle");
            dir->cd();
            auto particle_tree = new TTree(name.c_str(), "Reconstructed particle parameters");
            helper.root_dl2_rec_particle_map[name] = RootDL2RecParticle();
            helper.root_dl2_rec_particle_map[name]->initialize_write(particle_tree);
            trees[name] = particle_tree;
            directories[name] = dir;
        }
        auto root_dl2_rec_particle = helper.root_dl2_rec_particle_map[name].value();
        root_dl2_rec_particle.event_id = event.event_id;
        root_dl2_rec_particle = particle;
        particle_tree->Fill();
    }
    
    auto dl2_tree = get_tree("dl2");
    if(!dl2_tree)
    {
        spdlog::debug("initialize dl2");
        initialize_data_level("dl2", helper.root_dl2_camera);
        dl2_tree = get_tree("dl2");
    }
    auto& root_dl2_camera = helper.root_dl2_camera.value();
    root_dl2_camera.event_id = event.event_id;
    for(auto& [tid, dl2_tel] : dl2.tels)
    {
        root_dl2_camera.tel_id = tid;
        root_dl2_camera = dl2_tel;
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
        initialize_data_level("monitor", helper.root_tel_monitor);
        monitor_tree = get_tree("monitor");
    }
    
    const auto& monitor = event.monitor.value();
    auto& root_tel_monitor = helper.root_tel_monitor.value();
    root_tel_monitor.event_id = event.event_id;
    for(const auto& [tid, tel_monitor] : monitor.tels)
    {
        root_tel_monitor.tel_id = tid;
        root_tel_monitor = std::move(*tel_monitor);
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
        TDirectory* dir = get_or_create_directory("/events/");
        dir->cd();
        auto tree =  new TTree("pointing", "Array and telescope pointing information");
        pointing_tree = tree;
        helper.root_pointing = RootPointing();
        helper.root_pointing->initialize_write(pointing_tree);
        trees["pointing"] = pointing_tree;
        directories["pointing"] = dir;
    }
    const auto& pointing = event.pointing.value();
    auto& root_pointing = helper.root_pointing.value();
    // Variables for identification
    root_pointing.event_id = event.event_id;
    root_pointing = pointing;
    pointing_tree->Fill();
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
    TTreeSerializer::branch(&tree, config);
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
    auto tree = new TTree("tels", ("Telescope data for " + level_name).c_str());
    data_level->initialize_write(tree);
    trees[level_name] = tree;
    directories[level_name] = dir;
    build_index[level_name] = true;
}
void RootWriter::write_statistics(const Statistics& statistics, bool last)
{
    if(!file)
    {
        throw std::runtime_error("file not open");
    }
    static Statistics hist;
    if(!last)
    {
        hist += statistics;
        return;
    }
    TDirectory* dir = get_or_create_directory("statistics");
    dir->cd();
    // In ROOT Format, we just convert the histogram to TH1D Or TH2D
    int ihist = 0;
    for(const auto& [name, hist] : hist.histograms)
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
        else if(hist->get_dimension() == 0) // Profile1D
        {
            auto h1d = dynamic_cast<Profile1D<float>*>(hist.get());
            auto new_hist = new TProfile(("h" + std::to_string(ihist)).c_str(), name.c_str(), h1d->bins(), 
                                     h1d->get_low_edge(), h1d->get_high_edge());
            for(int i = 0; i < h1d->bins(); i++)
            {
                new_hist->SetBinContent(i+1, h1d->mean(i));
                new_hist->SetBinError(i+1, h1d->error(i));
            }
            new_hist->Write();
            ihist++;
        }
    }
}
