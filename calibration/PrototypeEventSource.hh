#pragma once

#include "CameraGeometry.hh"
#include "EventSource.hh"
#include "TFile.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeReaderArray.h"
#include <memory>
#include <string>

class PrototypeEventSource: public EventSource
{
public:
    PrototypeEventSource(const std::string& filename, const std::string camera_config_file="camera_geometry.root")
        : EventSource(filename), configfile(camera_config_file)
    {
        max_events = -1;
        load_simulated_showers = false;
        initialize_source();
    }
    virtual ~PrototypeEventSource() = default;
    /** Runs open_file, metadata readers, optional load_all_simulated_showers (Python / tooling). */
    void initialize_source() { initialize(); load_all_events(); }
    virtual void open_file() override;
    virtual void init_metaparam() override;
    virtual void init_atmosphere_model() override;
    virtual void init_subarray() override;
    virtual void init_simulation_config() override;
    virtual void load_all_simulated_showers() override;
    virtual ArrayEvent get_event() override;
    virtual ArrayEvent get_event(int index) override;
    virtual bool is_finished() override;
    void load_all_events();
    int mapping_fee_channel_to_pixels(short fee_id, unsigned char channel);
    static void set_config_path(std::string path) {
        configpath = path;
    }
    CameraGeometry read_camera_geometry();
private:
    std::unique_ptr<TFile> file;
    TTreeReader* waveforms_reader;
    TTreeReaderArray<Float_t>* ra_waveform;
    TTreeReaderValue<Int_t>* rv_event;
    TTreeReaderValue<unsigned short>* rv_fee_id;
    TTreeReaderValue<unsigned char>* rv_channel;
    TTreeReaderValue<Bool_t>* rv_hg_lg;
    TTreeReaderValue<UInt_t>* rv_rabbit_time;
    TTreeReaderValue<UInt_t>* rv_rabbittime;

    // In Calibration file, We just trying to read all data firstly
    std::vector<std::optional<ArrayEvent>> all_events;
    std::map<int, std::vector<int>> event_id_to_entries;
    std::optional<ArrayEvent> current_event;
    std::unordered_map<int, int> fee_channel_to_pixel_index;
    constexpr static int NUM_PIXELS = 1616;
    constexpr static int NUM_SAMPLES = 256;
    static std::string configpath;
    std::string configfile;

};