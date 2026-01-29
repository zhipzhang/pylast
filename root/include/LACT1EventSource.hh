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
inline MJDData get_mjd(int rabbitTime, double rabbittime) {
  // 先用整数和分数部分分开计算，避免double累积误差
  int seconds_in_day = 86400;
  double total_seconds = (rabbitTime + rabbittime * 8 / 1e9 - TAI2UTC);
  int days = static_cast<int>(total_seconds / seconds_in_day);
  int mjd_int = MJD19700101 + days;
  double mjd_double = (total_seconds - days * seconds_in_day) / seconds_in_day;
  if (mjd_double < 0) {
    mjd_int -= 1;
    mjd_double += 1.0;
  }
  return MJDData{mjd_int, mjd_double};
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
      return get_event(current_event_index++);
    } else {
      return ArrayEvent();
    }
  }
  void load_calibrate_event(ArrayEvent &event, int index);

private:
  std::unique_ptr<TFile> file;
  TTree *decode_tree = nullptr;
  std::unique_ptr<ROOT::RDataFrame> rdf_single_waveform;
  std::string decode_file_name;
  std::string camera_config_file;
  static constexpr int total_entries_per_event = 1616 * 2;
  struct LACT1ChannelData channel_data;
  LACT1CameraGeometry camera_geometry;
  int event_start;
  int event_end;
};