/**
 * @file DatabaseWriter.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Database writer for event data
 * @version 0.1
 * @date 2025-09-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <memory>
#include <string>
#include "duckdb.hpp"
#include "EventSource.hh"

class DatabaseWriter {
public:
    DatabaseWriter(const std::string& db_file);
    ~DatabaseWriter();
    
    void initialize();
    void writeEventData(EventSource& event_source, bool use_true = false);
    void clearTables();
    std::string db_file_;
    std::shared_ptr<duckdb::DuckDB> db_ptr() {
        return std::make_shared<duckdb::DuckDB>(db_file_);
    }
private:
    duckdb::DuckDB db_;
    duckdb::Connection conn_;
    
    void createTables();
    void writeSimulatedShowerData(duckdb::Appender& sim_shower_appender, const ArrayEvent& event, const std::string& source_file, size_t& sim_shower_count);
    void writeReconstructedEventData(duckdb::Appender& reco_event_appender, const ArrayEvent& event, const std::string& source_file, size_t& reco_event_count);
    void writeTelescopeData(duckdb::Appender& telescope_appender, const ArrayEvent& event, const std::string& source_file, size_t& tel_data_count, bool use_true = false);
};