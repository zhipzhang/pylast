/**
 * @file LACT1EventSource.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief
 * @version 0.1
 * @date 2026-01-25
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once
#include "EventSource.hh"
#include "LACT1CameraGeometry.hh"
#include "ROOT/RDataFrame.hxx"
#include "SubarrayDescription.hh"
#include "TFile.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include <memory>

struct LACT1ChannelData {
  int event;
  unsigned short borad_number;
  unsigned char channel_number;
  bool HG_LG;
  float base;
  float peak;
  float area;
  float rabbit_start;
  float rabbit_end;
};
constexpr int MJD19700101 = 40587;
constexpr int TAI2UTC = 37;
inline MJDData get_mjd(unsigned int rabbitTime, unsigned int rabbittime) {
  constexpr int64_t SECONDS_PER_DAY = 86400;
  constexpr int64_t NANOS_PER_SECOND = 1000000000LL;
  constexpr double NANOS_PER_SECOND_D = 1e9;
  
  int64_t nano_fraction = static_cast<int64_t>(rabbittime * 8.0);
  
  int64_t total_nanos = rabbitTime * NANOS_PER_SECOND + nano_fraction;
  
  int64_t tai_offset_nanos = static_cast<int64_t>(TAI2UTC * NANOS_PER_SECOND);
  total_nanos -= tai_offset_nanos;
  
  int64_t days = total_nanos / (SECONDS_PER_DAY * NANOS_PER_SECOND);
  int64_t day_nanos = total_nanos % (SECONDS_PER_DAY * NANOS_PER_SECOND);
  
  if (day_nanos < 0) {
      days -= 1;
      day_nanos += SECONDS_PER_DAY * NANOS_PER_SECOND;
  }
  
  int mjd_int = MJD19700101 + static_cast<int>(days);
  double mjd_double = static_cast<double>(day_nanos) / 
                     static_cast<double>(SECONDS_PER_DAY * NANOS_PER_SECOND);
  
  return MJDData{mjd_int, mjd_double, rabbitTime, rabbittime};
}
class LACT1EventSource : public EventSource {
public:
  LACT1EventSource() = default;
  LACT1EventSource(const std::string &filename,
                   const std::string &camera_config_file,
                   int64_t max_events = -1, std::vector<int> subarray = {},
                   bool load_simulated_showers = false);
  ~LACT1EventSource() = default;
  virtual void open_file() override;
  virtual void init_metaparam() override{};
  virtual void init_atmosphere_model() override{};
  virtual void init_subarray() override {
    subarray = SubarrayDescription();
    camera_geometry = LACT1CameraGeometry(camera_config_file);
    auto optical_description = OpticsDescription{.optics_name = "LACT1",
                                                 .num_mirrors = 1,
                                                 .mirror_area = 1,
                                                 .equivalent_focal_length = 8,
                                                 .effective_focal_length = 8};
    camera_geometry.pix_width_fov =
        camera_geometry.pix_width / optical_description.effective_focal_length;
    camera_geometry.pix_x_fov =
        camera_geometry.pix_x / optical_description.effective_focal_length;
    camera_geometry.pix_y_fov =
        camera_geometry.pix_y / optical_description.effective_focal_length;
    auto camera_description = CameraDescription{"LACT1", camera_geometry};

    auto telescope_description = TelescopeDescription{
        .camera_description = std::move(camera_description),
        .optics_description = OpticsDescription{.optics_name = "LACT1",
                                                .num_mirrors = 1,
                                                .mirror_area = 1,
                                                .equivalent_focal_length = 8,
                                                .effective_focal_length = 8}};
    subarray->add_telescope(0, std::move(telescope_description), {0, 0, 0});
  };
  virtual void init_simulation_config() override{};
  virtual void load_all_simulated_showers() override{};
  virtual ArrayEvent get_event(int index) override;
  virtual bool is_finished() override {
    return current_event_index >= max_events;
  }
  virtual ArrayEvent get_event() override {
    if (!is_finished()) {
      return get_event(current_event_index);
    } else {
      return ArrayEvent();
    }
  }
  void load_calibrate_event(ArrayEvent &event);

private:
  std::unique_ptr<TFile> file;
  TTree *decode_tree = nullptr;
  std::string decode_file_name;
  std::string camera_config_file;
  LACT1CameraGeometry camera_geometry;
// Temporay value reader for current version 2026.02.10
  std::unique_ptr<TTreeReaderValue<int>> event_number_reader;
  std::unique_ptr<TTreeReaderValue<unsigned int>> rabbit_Time_reader;
  std::unique_ptr<TTreeReaderValue<unsigned int>> rabbit_time_reader;
  std::unique_ptr<TTreeReaderValue<std::vector<int>>> map_number_reader;
  std::unique_ptr<TTreeReaderValue<std::vector<bool>>> HG_LG_reader;
  std::unique_ptr<TTreeReaderValue<std::vector<float>>> base_reader;
  std::unique_ptr<TTreeReaderValue<std::vector<float>>> peak_reader;
  std::unique_ptr<TTreeReaderValue<std::vector<float>>> area_reader;
  std::unique_ptr<TTreeReader> event_tree_reader;
};