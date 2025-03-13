#pragma once

#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
void initialize_logger(const std::string& log_level = "info", const std::string& log_file = "");
void shutdown_logger();