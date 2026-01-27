#include "LACT1CameraGeometry.hh"
#include "csv_parser.h"

// Static helper function to parse CSV and return camera data
LACT1CameraGeometry::CameraData
LACT1CameraGeometry::parse_camera_file(const std::string &file_name) {
  CameraData data;
  io::CSVReader<5> in(file_name);
  in.read_header(io::ignore_extra_column, "map_number", "s_number", "s_x",
                 "s_y", "fee_id");

  int map_number, s_number;
  double s_x, s_y;
  int fee_id;

  while (in.read_row(map_number, s_number, s_x, s_y, fee_id)) {
    data.map_number_to_pix_x[map_number] = s_x / 1000;
    data.map_number_to_pix_y[map_number] = s_y / 1000;
    data.pix_area.push_back(current_pix_area);
    data.pix_type.push_back(2); // Square pixels
    int channel_number = s_number % number_per_boards;
    if (channel_number == 0) {
      channel_number = 16;
    }
    data.map_to_board[map_number] = fee_id * number_per_boards + channel_number;
    data.board_to_map[fee_id * number_per_boards + channel_number] = map_number;
  }
  data.num_pixels = static_cast<int>(data.pix_area.size());
  for (int i = 1; i <= data.num_pixels; i++) {
    data.pix_x.push_back(data.map_number_to_pix_x[i]);
    data.pix_y.push_back(data.map_number_to_pix_y[i]);
  }
  return data;
}

// Private delegating constructor (takes CameraData by value)
LACT1CameraGeometry::LACT1CameraGeometry(const std::string &file_name,
                                         CameraData data)
    : CameraGeometry(default_camera_name, data.num_pixels, data.pix_x.data(),
                     data.pix_y.data(), data.pix_area.data(),
                     data.pix_type.data(), 0.0),
      camera_file_name(file_name), map_to_board_number(data.map_to_board),
      board_to_map_number(data.board_to_map) {}

// Public constructor: parse file first, then delegate to private constructor
LACT1CameraGeometry::LACT1CameraGeometry(const std::string &file_name)
    : LACT1CameraGeometry(file_name, parse_camera_file(file_name)) {}

std::pair<int, int>
LACT1CameraGeometry::convert_map_to_board_channel(int map_number) {
  int board_number = map_to_board_number[map_number] / number_per_boards;
  int channel_number = map_to_board_number[map_number] % number_per_boards;
  if (channel_number == 0) {
    channel_number = 16;
  }
  return std::make_pair(board_number, channel_number);
}

int LACT1CameraGeometry::convert_board_channel_to_map(int board_number,
                                                      int channel_number) {
  if (channel_number == 0) {
    channel_number = 16;
  }
  return board_to_map_number[board_number * number_per_boards + channel_number];
}