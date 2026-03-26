#include "PrototypeEventSource.hh"
#include "C0Event.hh"
#include "CameraDescription.hh"
#include "CameraReadout.hh"
#include "TTree.h"
#include <optional>
#include <stdexcept>
#include <string>
#include "TFile.h"
#include "spdlog/spdlog.h"

namespace {
const std::string kIhepEosUrl = "root://eos01.ihep.ac.cn:/";
}

std::string PrototypeEventSource::configpath = "./config";

void PrototypeEventSource::open_file()
{
    if(input_filename.size() >= 4 && input_filename.substr(0, 4) == "/eos")
    {
        input_filename = kIhepEosUrl + input_filename;
    }

    file = std::unique_ptr<TFile>(TFile::Open(input_filename.c_str(), "READ"));
    if(!file || file->IsZombie())
    {
        throw std::runtime_error("file not found: " + input_filename);
    }

    auto* waveforms_tree = static_cast<TTree*>(file->Get("waveforms"));
    if(!waveforms_tree)
    {
        throw std::runtime_error("TTree 'waveforms' not found in " + input_filename);
    }
    waveforms_reader = new TTreeReader("waveforms", file.get());
    ra_waveform = new TTreeReaderArray<Float_t>(*waveforms_reader, "waveform");
    rv_event = new TTreeReaderValue<Int_t>(*waveforms_reader, "event");
    rv_fee_id = new TTreeReaderValue<unsigned short>(*waveforms_reader, "FEE_ID");
    rv_channel = new TTreeReaderValue<unsigned char>(*waveforms_reader, "channel");
    rv_hg_lg = new TTreeReaderValue<Bool_t>(*waveforms_reader, "HG_LG");
    rv_rabbit_time = new TTreeReaderValue<UInt_t>(*waveforms_reader, "RabbitTime");
    rv_rabbittime = new TTreeReaderValue<UInt_t>(*waveforms_reader, "Rabbittime");
}

void PrototypeEventSource::load_all_events()
{
    std::cout << "Loading all events..." << std::endl;
    
    // 使用哈希表在内存中缓存正在构建的 Event
    std::map<int, std::unique_ptr<ArrayEvent>> event_map;

    // 顺序遍历！这是最快的方式
    while (waveforms_reader->Next()) 
    {
        int event_id = **rv_event;
        
        // 如果这个 event_id 是第一次遇到，初始化它
        if (!event_map.contains(event_id)) {
            auto new_event= std::make_unique<ArrayEvent>();
            new_event->event_id = event_id;
            
            C0Camera c0_camera;
            c0_camera.high_gain_waveform.resize(NUM_PIXELS, NUM_SAMPLES);
            c0_camera.low_gain_waveform.resize(NUM_PIXELS, NUM_SAMPLES);
            c0_camera.n_pixels = NUM_PIXELS;
            c0_camera.n_samples = NUM_SAMPLES;
            
            C0Event c0_event;
            c0_event.add_tel(1, std::move(c0_camera));
            new_event->c0 = std::move(c0_event);
            
            event_map[event_id] = std::move(new_event);
        }

        // 获取并填充当前条目的波形
        int pixel_index = mapping_fee_channel_to_pixels(**rv_fee_id, **rv_channel) - 1;
        if(pixel_index < 0) continue;
        if (pixel_index >= NUM_PIXELS)
        {
            spdlog::error("pixel_index out of range: {}", pixel_index);
            continue;
        }

        auto& c0_cam = event_map[event_id]->c0->tels[1];
        if(*rv_hg_lg->Get()) {
            for(int i = 0; i < NUM_SAMPLES; ++i) {
                c0_cam->high_gain_waveform(pixel_index, i) = (*ra_waveform)[i];
            }
        } else {
            for(int i = 0; i < NUM_SAMPLES; ++i) {
                c0_cam->low_gain_waveform(pixel_index, i) = (*ra_waveform)[i];
            }
        }
    }

    for (auto& [event_id, event] : event_map) {
        all_events.push_back(std::make_optional<ArrayEvent>(std::move(*event)));
    }
    
    max_events = static_cast<int64_t>(all_events.size());
    std::cout << "Finish loading all events" << std::endl;
}

CameraGeometry PrototypeEventSource::read_camera_geometry()
{
    std::string camera_config_file = configpath + "/" + configfile;
    std::unique_ptr<TFile> config_file = std::unique_ptr<TFile>(TFile::Open(camera_config_file.c_str(), "READ"));
    if(!config_file)
    {
        throw std::runtime_error("camera config file not found: " + camera_config_file);
    }
    auto camera_geometry_tree = static_cast<TTree*>(config_file->Get("camera_geometry"));
    if(!camera_geometry_tree)
    {
        throw std::runtime_error("camera geometry tree not found in " + camera_config_file);
    }
    int ipix_number;
    int ifee_id;
    int ichannel;
    double ipix_x;
    double ipix_y;
    camera_geometry_tree->SetBranchAddress("pixel_number", &ipix_number);
    camera_geometry_tree->SetBranchAddress("fee_id", &ifee_id);
    camera_geometry_tree->SetBranchAddress("channel", &ichannel);
    camera_geometry_tree->SetBranchAddress("pix_x", &ipix_x);
    camera_geometry_tree->SetBranchAddress("pix_y", &ipix_y);
    Eigen::VectorXd camera_pix_x(NUM_PIXELS);
    Eigen::VectorXd camera_pix_y(NUM_PIXELS);
    Eigen::VectorXd camera_pix_area(NUM_PIXELS);
    Eigen::VectorXi camera_pix_type(NUM_PIXELS);
    camera_pix_type.setConstant(2);
    camera_pix_area.setConstant(0.0244 * 0.0244);
    for(int i = 0; i < camera_geometry_tree->GetEntries(); ++i)
    {
        camera_geometry_tree->GetEntry(i);
        fee_channel_to_pixel_index[ifee_id * 16 + ichannel] = ipix_number;
        camera_pix_x(ipix_number - 1) = ipix_x;
        camera_pix_y(ipix_number - 1) = ipix_y;
    }
    auto camera_geometry = CameraGeometry("prototype_geometry", NUM_PIXELS, std::move(camera_pix_x), std::move(camera_pix_y), std::move(camera_pix_area), std::move(camera_pix_type), 0);
    std::cout << "Finish reading camera geometry" << std::endl;
    return camera_geometry;
}

void PrototypeEventSource::init_subarray()
{
    auto camera_geometry = read_camera_geometry();
    subarray = SubarrayDescription();
    auto camera_description = CameraDescription{.camera_name="prototype_camera", .camera_geometry=std::move(camera_geometry), .camera_readout=CameraReadout{}};
    auto optics_description = OpticsDescription{.optics_name="prototype_optics", .num_mirrors=37, .mirror_area=100, .equivalent_focal_length = 8.15, .effective_focal_length=8.15 };
    subarray->add_telescope(1, TelescopeDescription{.camera_description=std::move(camera_description), .optics_description=std::move(optics_description)}, {0, 0, 0});
}

void PrototypeEventSource::init_metaparam() {}

void PrototypeEventSource::init_atmosphere_model() {}

void PrototypeEventSource::init_simulation_config() {}

void PrototypeEventSource::load_all_simulated_showers()
{
}

int PrototypeEventSource::mapping_fee_channel_to_pixels(short fee_id, unsigned char channel)
{
    if(fee_id == 220)
    {
        fee_id =80;
    }
    if(fee_id == 219)
    {
        fee_id = 85;
    }
    if(fee_id == 221)
    {
        fee_id = 10;
    }

    const int key = static_cast<int>(fee_id) * 16 + static_cast<int>(channel);
    const auto it = fee_channel_to_pixel_index.find(key);
    if(it == fee_channel_to_pixel_index.end())
    {
        spdlog::error("Fee channel {} not found in camera geometry", key);
        return -1;
    }
    return it->second;
}

ArrayEvent PrototypeEventSource::get_event()
{
    if(is_finished())
    {
        return ArrayEvent();
    }
    return get_event(static_cast<int>(current_event_index));
}

ArrayEvent PrototypeEventSource::get_event(int index)
{
    if(index < 0 || static_cast<size_t>(index) >= all_events.size()) {
        throw std::out_of_range("Index out of range");
    }

    auto& opt_event = all_events[static_cast<size_t>(index)];
    
    if (!opt_event.has_value()) {
        throw std::runtime_error("Event at index " + std::to_string(index) + " has already been accessed");
    }

    ArrayEvent extracted_event = std::move(*opt_event); 
    
    opt_event.reset(); 

    return extracted_event; 
}
bool PrototypeEventSource::is_finished()
{
    return max_events <= 0 || current_event_index >= max_events;
}