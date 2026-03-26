#pragma once

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
    PrototypeEventSource(const std::string& filename, const std::string& mapping_file)
        : EventSource(filename), mapping_file(mapping_file) {}
    virtual ~PrototypeEventSource() = default;
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
    int mapping_fee_channel_to_pixels(short fee_id, int channel);
private:
    std::string mapping_file;

    std::unique_ptr<TFile> file;
    std::unique_ptr<TTreeReader> waveforms_reader;
    std::unique_ptr<TTreeReaderArray<Float_t>> ra_waveform;
    std::unique_ptr<TTreeReaderValue<Int_t>> rv_event;
    std::unique_ptr<TTreeReaderValue<Short_t>> rv_fee_id;
    std::unique_ptr<TTreeReaderValue<Char_t>> rv_channel;
    std::unique_ptr<TTreeReaderValue<Bool_t>> rv_hg_lg;
    std::unique_ptr<TTreeReaderValue<UChar_t>> rv_fpga_id;
    std::unique_ptr<TTreeReaderValue<UInt_t>> rv_rabbit_time;
    std::unique_ptr<TTreeReaderValue<UInt_t>> rv_rabbittime;

    // In Calibration file, We just trying to read all data firstly
    std::vector<ArrayEvent> all_events;
    std::map<int, std::vector<int>> event_id_to_entries;
    constexpr static int NUM_PIXELS = 1616;
    constexpr static int NUM_SAMPLES = 256;
};