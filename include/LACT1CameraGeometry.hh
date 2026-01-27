/**
 * @file LACT1CameraGeometry.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief LACT first Telescope's Geometry, have extra parameters handle for
 * board/channel
 * @version 0.1
 * @date 2026-01-25
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "CameraGeometry.hh"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class LACT1CameraGeometry : public CameraGeometry {
public:
  LACT1CameraGeometry() = default;
  LACT1CameraGeometry(const std::string &file_name);
  ~LACT1CameraGeometry() = default;

  std::string camera_file_name;
  std::pair<int, int> convert_map_to_board_channel(int map_number);
  int convert_board_channel_to_map(int board_number, int channel_number);

private:
  // Helper struct to hold parsed CSV data
  struct CameraData {
    std::vector<double> pix_x;
    std::vector<double> pix_y;
    std::vector<double> pix_area;
    std::vector<int> pix_type;
    std::unordered_map<int, int> map_to_board;
    std::unordered_map<int, int> board_to_map;
    std::unordered_map<int, double> map_number_to_pix_x;
    std::unordered_map<int, double> map_number_to_pix_y;
    int num_pixels;
  };

  // Static helper function to parse CSV file
  static CameraData parse_camera_file(const std::string &file_name);

  // Private delegating constructor
  LACT1CameraGeometry(const std::string &file_name, CameraData data);

  std::unordered_map<int, int> map_to_board_number;
  std::unordered_map<int, int> board_to_map_number;
  static constexpr int number_per_boards = 16;
  static constexpr int max_boards = 101;
  static constexpr double current_pix_area =
      (25.4 * 25.4) / 1000000; // Convert mm^2 to m^2
  static constexpr const char *default_camera_name = "LACT1";
};