#include "LoggerInitialize.hh"

void initialize_logger(const std::string& log_level, const std::string& log_file) {
    // 创建控制台 sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::from_str(log_level));
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    // 创建一个包含 console_sink 的 sinks vector
    std::vector<spdlog::sink_ptr> sinks{console_sink};

    // 如果指定了日志文件路径，创建文件 sink
    if (!log_file.empty()) {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
        file_sink->set_level(spdlog::level::from_str(log_level));
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        sinks.push_back(file_sink);
    }

    // 创建 logger 并注册为全局 logger
    auto logger = std::make_shared<spdlog::logger>("multi_sink_logger", begin(sinks), end(sinks));
    logger->set_level(spdlog::level::from_str(log_level));
    logger->flush_on(spdlog::level::from_str(log_level));

    spdlog::set_default_logger(logger);

    spdlog::info("Logger initialized with level: {}", log_level);
    if (!log_file.empty()) {
        spdlog::info("Logging to file: {}", log_file);
    } else {
        spdlog::info("File logging is disabled");
    }
}