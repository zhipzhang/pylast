#include "PrototypeEventSource.hh"
#include "C0Event.hh"
#include "TTree.h"
#include <stdexcept>
#include <string>

namespace {
const std::string kIhepEosUrl = "root://eos01.ihep.ac.cn:/";
}

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
    int tmp_event_id = 0;
    waveforms_tree->SetBranchAddress("event_id", &tmp_event_id);
    for(int i = 0; i < waveforms_tree->GetEntries(); ++i)
    {
        waveforms_tree->GetEntry(i);
        event_id_to_entries[tmp_event_id].push_back(i);
    }
    waveforms_reader = std::make_unique<TTreeReader>("waveforms", file.get());
    ra_waveform = std::make_unique<TTreeReaderArray<Float_t>>(*waveforms_reader, "waveform");
    rv_event = std::make_unique<TTreeReaderValue<Int_t>>(*waveforms_reader, "event");
    rv_fee_id = std::make_unique<TTreeReaderValue<Short_t>>(*waveforms_reader, "FEE_ID");
    rv_channel = std::make_unique<TTreeReaderValue<Char_t>>(*waveforms_reader, "channel");
    rv_hg_lg = std::make_unique<TTreeReaderValue<Bool_t>>(*waveforms_reader, "HG_LG");
    rv_fpga_id = std::make_unique<TTreeReaderValue<UChar_t>>(*waveforms_reader, "fpga_id");
    rv_rabbit_time = std::make_unique<TTreeReaderValue<UInt_t>>(*waveforms_reader, "RabbitTime");
    rv_rabbittime = std::make_unique<TTreeReaderValue<UInt_t>>(*waveforms_reader, "Rabbittime");
    
}

void PrototypeEventSource::load_all_events()
{
    for(const auto& [event_id, entries] : event_id_to_entries)
    {
        ArrayEvent event;
        event.event_id = event_id;
        C0Camera c0_camera;
        c0_camera.high_gain_waveform.resize(NUM_PIXELS, NUM_SAMPLES);
        c0_camera.low_gain_waveform.resize(NUM_PIXELS, NUM_SAMPLES);
        c0_camera.n_pixels = NUM_PIXELS;
        c0_camera.n_samples = NUM_SAMPLES;
        for(const auto& entry : entries)
        {
            waveforms_reader->SetEntry(entry);
            int pixel_index = mapping_fee_channel_to_pixels(*rv_fee_id->Get(), *rv_channel->Get());
            if(rv_hg_lg->Get())
            {
                c0_camera.high_gain_waveform.row(pixel_index) = Eigen::VectorXf(ra_waveform->begin(), ra_waveform->end());
            }
            else
            {
                c0_camera.low_gain_waveform.row(pixel_index) = Eigen::VectorXf(ra_waveform->begin(), ra_waveform->end());
            }
        }
        C0Event c0_event;
        c0_event.add_tel(1, std::move(c0_camera));
        event.c0 = std::move(c0_event);
        all_events.push_back(std::move(event));
    }
}