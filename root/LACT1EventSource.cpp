#include "LACT1EventSource.hh"
#include "spdlog/spdlog.h"
#include <set>
LACT1EventSource::LACT1EventSource(const std::string &filename,
                                   const std::string &camera_config_file,
                                   int64_t max_events,
                                   std::vector<int> subarray,
                                   bool load_simulated_showers)
    : EventSource(filename, max_events, subarray, load_simulated_showers),
      camera_config_file(camera_config_file) {
  initialize();
}
const std::string ihep_url = "root://eos01.ihep.ac.cn:/";

void LACT1EventSource::open_file() {
  decode_file_name = input_filename;
  spdlog::info("LACT1EventSource open file: {}", input_filename);
  if (input_filename.substr(0, 4) == "/eos") {
    decode_file_name = ihep_url + input_filename;
  } else {
    decode_file_name = input_filename;
  }
  file = std::make_unique<TFile>(decode_file_name.c_str(), "READ");
  if (!file) {
    throw std::runtime_error("Failed to open decode file: " + decode_file_name);
  }
  decode_tree = static_cast<TTree *>(file->Get("singleWaveform"));
  if (!decode_tree) {
    throw std::runtime_error("Failed to open singleWaveform: " +
                             decode_file_name);
  }
  max_events = decode_tree->GetEntries() / total_entries_per_event;

  rdf_single_waveform =
      std::make_unique<ROOT::RDataFrame>("singleWaveform", decode_file_name);
  event_start = rdf_single_waveform->Min<int>("event").GetValue();
  event_end = rdf_single_waveform->Max<int>("event").GetValue();
  if (event_end - event_start + 1 != max_events) {
    throw std::runtime_error(
        "Event range mismatch: " + std::to_string(event_start) + " != 0 or " +
        std::to_string(event_end) + " != " + std::to_string(max_events - 1));
  }
}

ArrayEvent LACT1EventSource::get_event(int index) {
  if (index < 0 || index >= max_events) {
    throw std::out_of_range("Index out of range: " + std::to_string(index));
  }
  ArrayEvent event;
  event.event_id = index + event_start;
  event.c1 = CalibEvent();
  load_calibrate_event(event, index + event_start);
  return event;
}

void LACT1EventSource::load_calibrate_event(ArrayEvent &event, int index) {
  auto event_df = rdf_single_waveform->Filter(
      [index](int event) { return event == index; }, {"event"});
  auto HG_LG_df = event_df.Take<bool>("HG_LG");
  auto base_df = event_df.Take<float>("base");
  auto peak_df = event_df.Take<float>("amp");
  auto area_df = event_df.Take<float>("area");
  auto map_number = event_df.Take<int>("map_number");
  int rabittime1 = event_df.Take<int>("RabbitTime")->at(0);
  int rabbittime2 = event_df.Take<int>("Rabbittime")->at(0);

  CalibCamera calib_camera;
  calib_camera.base.resize(2, camera_geometry.num_pixels);
  calib_camera.peak.resize(2, camera_geometry.num_pixels);
  calib_camera.area.resize(2, camera_geometry.num_pixels);
  calib_camera.count_to_pe.resize(2, camera_geometry.num_pixels);
  for (int i = 0; i < HG_LG_df->size(); i++) {
    if (HG_LG_df->at(i)) {
      calib_camera.base(0, map_number->at(i) - 1) = base_df->at(i);
      calib_camera.peak(0, map_number->at(i) - 1) = peak_df->at(i);
      calib_camera.area(0, map_number->at(i) - 1) = area_df->at(i);
    } else {
      calib_camera.base(1, map_number->at(i) - 1) = base_df->at(i);
      calib_camera.peak(1, map_number->at(i) - 1) = peak_df->at(i);
      calib_camera.area(1, map_number->at(i) - 1) = area_df->at(i);
    }
  }
  event.c1->add_tel(0, std::move(calib_camera));
}