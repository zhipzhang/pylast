#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "SimtelFileHandler.hh"
#include <spdlog/spdlog.h>
#include "SimtelEventSource.hh"
#include "LoggerInitialize.hh"
int main(int argc, char** argv) {
    initialize_logger("debug", "debug.log");
    auto simtel_source = new SimtelEventSource(argv[1]);
    simtel_source->load_all_simulated_showers();
}
