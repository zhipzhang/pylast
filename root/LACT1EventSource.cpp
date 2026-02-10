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
  file = std::unique_ptr<TFile>(TFile::Open(decode_file_name.c_str(), "READ"));
  if (!file) {
    throw std::runtime_error("Failed to open decode file: " + decode_file_name);
  }
  decode_tree = static_cast<TTree *>(file->Get("eventShow"));
  if (!decode_tree) {
    throw std::runtime_error("Failed to open event_show: " +
                             decode_file_name);
  }
  max_events = decode_tree->GetEntries();
  event_tree_reader = std::unique_ptr<TTreeReader>(new TTreeReader(decode_tree));
  event_number_reader = std::make_unique<TTreeReaderValue<int>>(*event_tree_reader, "event");
  rabbit_Time_reader = std::make_unique<TTreeReaderValue<unsigned int>>(*event_tree_reader, "RabbitTime");
  rabbit_time_reader = std::make_unique<TTreeReaderValue<unsigned int>>(*event_tree_reader, "Rabbittime");
  map_number_reader = std::make_unique<TTreeReaderValue<std::vector<int>>>(*event_tree_reader, "map_number");
  HG_LG_reader = std::make_unique<TTreeReaderValue<std::vector<bool>>>(*event_tree_reader, "HG_LG");
  base_reader = std::make_unique<TTreeReaderValue<std::vector<float>>>(*event_tree_reader, "base");
  peak_reader = std::make_unique<TTreeReaderValue<std::vector<float>>>(*event_tree_reader, "amp");
  area_reader = std::make_unique<TTreeReaderValue<std::vector<float>>>(*event_tree_reader, "area");
}

ArrayEvent LACT1EventSource::get_event(int index) {
  if (index < 0 || index >= max_events) {
    throw std::out_of_range("Index out of range: " + std::to_string(index));
  }
  event_tree_reader->SetEntry(index);
  event_tree_reader->Next();
  ArrayEvent event;
  event.event_id = *(*event_number_reader);
  event.c1 = CalibEvent();
  load_calibrate_event(event);
  return event;
}

void LACT1EventSource::load_calibrate_event(ArrayEvent &event) {
  unsigned int rabittime1 = *(*rabbit_Time_reader);
  unsigned int rabbittime2 = *(*rabbit_time_reader);
  event.mjd = get_mjd(rabittime1, rabbittime2);
  CalibCamera calib_camera;
  calib_camera.base.resize(2, camera_geometry.num_pixels);
  calib_camera.peak.resize(2, camera_geometry.num_pixels);
  calib_camera.area.resize(2, camera_geometry.num_pixels);
  calib_camera.count_to_pe.resize(2, camera_geometry.num_pixels);
  for (int i = 0; i < (*HG_LG_reader)->size(); i++) {
    if ((*HG_LG_reader)->at(i)) {
      calib_camera.base(0, (*map_number_reader)->at(i) - 1) = (*base_reader)->at(i);
      calib_camera.peak(0, (*map_number_reader)->at(i) - 1) = (*peak_reader)->at(i);
      calib_camera.area(0, (*map_number_reader)->at(i) - 1) = (*area_reader)->at(i);
    } else {
      calib_camera.base(1, (*map_number_reader)->at(i) - 1) = (*base_reader)->at(i);
      calib_camera.peak(1, (*map_number_reader)->at(i) - 1) = (*peak_reader)->at(i);
      calib_camera.area(1, (*map_number_reader)->at(i) - 1) = (*area_reader)->at(i);
    }
  }
  event.c1->add_tel(0, std::move(calib_camera));
}