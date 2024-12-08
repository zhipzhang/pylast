#include "LoggerInitialize.hh"
#include "spdlog/async_logger.h"
#include "spdlog/async.h"
#include "spdlog/common.h"

static std::shared_ptr<spdlog::details::thread_pool> global_thread_pool;
void initialize_logger(const std::string& log_level, const std::string& log_file);
void initialize_logger(const std::string& log_level, const std::string& log_file) {
    // Create async console sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::err);
    console_sink->set_pattern("[%^%l%$] %v");
    
    // Create thread pool
    global_thread_pool = std::make_shared<spdlog::details::thread_pool>(81920, 1);
    
    std::shared_ptr<spdlog::async_logger> logger;
    if(!log_file.empty()) {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
        file_sink->set_level(spdlog::level::trace);
        if(log_level == "trace" || log_level == "debug") {
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
        } else {
            file_sink->set_pattern("[%^%l%$] %v");
        }
        logger = std::make_shared<spdlog::async_logger>("multi_sink_logger", 
            spdlog::sinks_init_list{file_sink, console_sink},
            global_thread_pool, spdlog::async_overflow_policy::overrun_oldest);
    } else {
        logger = std::make_shared<spdlog::async_logger>("multi_sink_logger",
            spdlog::sinks_init_list{console_sink},
            global_thread_pool);
    }
    logger->set_level(spdlog::level::from_str(log_level));
    logger->flush_on(spdlog::level::warn);

    // Set as default logger
    spdlog::set_default_logger(logger);

    spdlog::info("Logger initialized with level: {}", log_level);
    if (!log_file.empty()) {
        spdlog::info("Logging to file: {}", log_file);
    } else {
        spdlog::info("File logging is disabled");
    }
}

void shutdown_logger() {
    // Flush any remaining logs
    spdlog::default_logger()->flush();
    // Drop all loggers and close threadpool
    spdlog::shutdown();
    // Reset thread pool
    global_thread_pool.reset();
}
