/**
 * @file DatabaseWriter.cpp
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Database writer for event data
 * @version 0.1
 * @date 2025-09-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "DatabaseWriter.hh"
#include "ImageParameters.hh"
#include "SimulatedShower.hh"
#include "DL2Event.hh"
#include "DL1Event.hh"
#include "duckdb.hpp"
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <variant>
#include "spdlog/spdlog.h"

using namespace duckdb;

// Batch size for flushing data to database
static const size_t BATCH_SIZE = 81920;

DatabaseWriter::DatabaseWriter(const std::string& db_file) 
    : db_file_(db_file), db_(db_file), conn_(db_) {
        initialize();
}

DatabaseWriter::~DatabaseWriter() {
}

void DatabaseWriter::initialize() {
    auto result = conn_.Query("SELECT version() AS version");
    if (!result->HasError() && result->RowCount() > 0) {
    auto version_value = result->GetValue(0, 0);  // First row, first column
    std::string version = version_value.ToString();
    spdlog::info("DuckDB Version: {}", version);
    } else {
        spdlog::error("Error retrieving DuckDB version: {}", result->GetError());
        throw std::runtime_error("Failed to retrieve DuckDB version");
    }
    createTables();
}

void DatabaseWriter::createTables() {
    // Create SimulatedShower table
    conn_.Query("CREATE TABLE IF NOT EXISTS SimulatedShower ("
                "event_id INTEGER, "
                "run_id INTEGER, "
                "source_file VARCHAR, "
                "true_energy DOUBLE, "
                "true_alt DOUBLE, "
                "true_az DOUBLE, "
                "true_core_x DOUBLE, "
                "true_core_y DOUBLE, "
                "true_x_max DOUBLE, "
                "true_h_max DOUBLE, "
                "pointing_alt DOUBLE, "
                "pointing_az DOUBLE)");

    // Create ReconstructedEvent table
    conn_.Query("CREATE TABLE IF NOT EXISTS ReconstructedEvent ("
                "event_id INTEGER, "
                "run_id INTEGER, "
                "source_file VARCHAR, "
                "reco_alt DOUBLE, "
                "reco_alt_uncertainty DOUBLE, "
                "reco_az DOUBLE, "
                "reco_az_uncertainty DOUBLE, "
                "reco_core_x DOUBLE, "
                "reco_core_y DOUBLE, "
                "reco_core_pos_error DOUBLE, "
                "reco_hmax DOUBLE, "
                "reco_xmax DOUBLE, "
                "reco_energy DOUBLE, "
                "reco_hadroness DOUBLE, "
                "multiplicity INTEGER)");

    // Create Telescope table
    conn_.Query("CREATE TABLE IF NOT EXISTS Telescope ("
                "event_id INTEGER, "
                "run_id INTEGER, "
                "tel_id INTEGER, "
                "source_file VARCHAR, "
                "true_impact_parameter DOUBLE, "
                "hillas_intensity DOUBLE, "
                "hillas_width DOUBLE, "
                "hillas_length DOUBLE, "
                "hillas_psi DOUBLE, "
                "hillas_x DOUBLE, "
                "hillas_y DOUBLE, "
                "hillas_skewness DOUBLE, "
                "hillas_kurtosis DOUBLE, "
                "hillas_r DOUBLE, "
                "hillas_phi DOUBLE, "
                "leakage_pixels_width_1 DOUBLE, "
                "leakage_pixels_width_2 DOUBLE, "
                "leakage_intensity_width_1 DOUBLE, "
                "leakage_intensity_width_2 DOUBLE, "
                "concentration_cog DOUBLE, "
                "concentration_core DOUBLE, "
                "concentration_pixel DOUBLE, "
                "morphology_n_pixels INTEGER, "
                "morphology_n_islands INTEGER, "
                "morphology_n_small_islands INTEGER, "
                "morphology_n_medium_islands INTEGER, "
                "morphology_n_large_islands INTEGER, "
                "intensity_max DOUBLE, "
                "intensity_mean DOUBLE, "
                "intensity_std DOUBLE, "
                "intensity_skewness DOUBLE, "
                "intensity_kurtosis DOUBLE, "
                "extra_miss DOUBLE, "
                "extra_disp DOUBLE, "
                "extra_theta DOUBLE, "
                "extra_true_psi DOUBLE, "
                "extra_cog_err DOUBLE, "
                "extra_beta_err DOUBLE, "
                "reco_impact_parameter DOUBLE,"
                "time_range_10_90 DOUBLE, )");
}

void DatabaseWriter::writeEventData(EventSource& event_source, bool use_true) {
    // Use the input_filename from EventSource as the source_file
    std::string source_file = event_source.input_filename;
    
    // Start a transaction for better performance
    auto result = conn_.Query("BEGIN TRANSACTION");
    if (result->HasError()) {
        std::cerr << "Error starting transaction: " << result->GetError() << std::endl;
        return;
    }
    
    // Create long-lived appenders for each table
    Appender sim_shower_appender(conn_, "SimulatedShower");
    Appender reco_event_appender(conn_, "ReconstructedEvent");
    Appender telescope_appender(conn_, "Telescope");
    
    // Counters for batch flushing
    size_t sim_shower_count = 0;
    size_t reco_event_count = 0;
    size_t tel_data_count = 0;
    
    // Iterate through all events in the event source
    for (const auto& event : event_source) {
        // Write simulated shower data
        writeSimulatedShowerData(sim_shower_appender, event, source_file, sim_shower_count);
        
        // Write reconstructed event data
        writeReconstructedEventData(reco_event_appender, event, source_file, reco_event_count);
        
        // Write telescope data
        writeTelescopeData(telescope_appender, event, source_file, tel_data_count, use_true);
    }
    
    // Final flush for any remaining data
    sim_shower_appender.Flush();
    reco_event_appender.Flush();
    telescope_appender.Flush();
    
    // Close appenders
    sim_shower_appender.Close();
    reco_event_appender.Close();
    telescope_appender.Close();
    
    // Commit the transaction
    result = conn_.Query("COMMIT");
    spdlog::info("Finished writing events to database: {}", source_file);
    if (result->HasError()) {
        std::cerr << "Error committing transaction: " << result->GetError() << std::endl;
        return;
    }
}

void DatabaseWriter::writeSimulatedShowerData(duckdb::Appender& sim_shower_appender, const ArrayEvent& event, const std::string& source_file, size_t& sim_shower_count) {
    if (event.simulation.has_value()) {
        // Prepare data for SimulatedShower table
        const auto& simulated_event = event.simulation.value();
        
        sim_shower_appender.BeginRow();
        sim_shower_appender.Append<int32_t>(event.event_id);
        sim_shower_appender.Append<int32_t>(event.run_id);
        sim_shower_appender.Append<const char*>(source_file.c_str());
        sim_shower_appender.Append<double>(simulated_event.shower.energy);
        sim_shower_appender.Append<double>(simulated_event.shower.alt);
        sim_shower_appender.Append<double>(simulated_event.shower.az);
        sim_shower_appender.Append<double>(simulated_event.shower.core_x);
        sim_shower_appender.Append<double>(simulated_event.shower.core_y);
        sim_shower_appender.Append<double>(simulated_event.shower.x_max);
        sim_shower_appender.Append<double>(simulated_event.shower.h_max);
        sim_shower_appender.Append<double>(event.pointing->array_altitude);
        sim_shower_appender.Append<double>(event.pointing->array_azimuth);
        sim_shower_appender.EndRow();
        
        sim_shower_count++;
        
        // Flush batch if needed
        if (sim_shower_count >= BATCH_SIZE) {
            spdlog::info("Flushing  simulated shower records to database");
            sim_shower_appender.Flush();
            sim_shower_count = 0;
        }
    }
}

void DatabaseWriter::writeReconstructedEventData(duckdb::Appender& reco_event_appender, const ArrayEvent& event, const std::string& source_file, size_t& reco_event_count) {
    if (event.dl2.has_value()) {
        const auto& dl2_event = event.dl2.value();
        
        // For now, we assume there's only one geometry reconstructor (HillasReconstructor)
        // In the future, we might need to handle multiple reconstructors
        if (!dl2_event.geometry.empty()) {
            // Get the first (and likely only) geometry reconstructor
            const auto& geometry_pair = *dl2_event.geometry.begin();
            const auto& reconstructed_geometry = geometry_pair.second;
            if(!reconstructed_geometry.is_valid)
            {
                // Skip invalid reconstructions
                return;
            }
            
            // Get energy and particle information if available
            double reco_energy = std::numeric_limits<double>::quiet_NaN();
            bool energy_valid = false;
            double reco_hadroness = std::numeric_limits<double>::quiet_NaN();
            bool particle_valid = false;
            
            if (!dl2_event.energy.empty()) {
                const auto& energy_pair = *dl2_event.energy.begin();
                const auto& reconstructed_energy = energy_pair.second;
                if (reconstructed_energy.energy_valid) {
                    reco_energy = reconstructed_energy.estimate_energy;
                    energy_valid = true;
                }
            }
            
            if (!dl2_event.particle.empty()) {
                const auto& particle_pair = *dl2_event.particle.begin();
                const auto& reconstructed_particle = particle_pair.second;
                if (reconstructed_particle.is_valid) {
                    reco_hadroness = reconstructed_particle.hadroness;
                    particle_valid = true;
                }
            }
            
            reco_event_appender.BeginRow();
            reco_event_appender.Append<int32_t>(event.event_id);
            reco_event_appender.Append<int32_t>(event.run_id);
            reco_event_appender.Append<const char*>(source_file.c_str());
            reco_event_appender.Append<double>(reconstructed_geometry.alt);
            reco_event_appender.Append<double>(reconstructed_geometry.alt_uncertainty);
            reco_event_appender.Append<double>(reconstructed_geometry.az);
            reco_event_appender.Append<double>(reconstructed_geometry.az_uncertainty);
            reco_event_appender.Append<double>(reconstructed_geometry.core_x);
            reco_event_appender.Append<double>(reconstructed_geometry.core_y);
            reco_event_appender.Append<double>(reconstructed_geometry.core_pos_error);
            reco_event_appender.Append<double>(reconstructed_geometry.hmax);
            reco_event_appender.Append<double>(reconstructed_geometry.xmax);
            
            if (energy_valid) {
                reco_event_appender.Append<double>(reco_energy);
            } else {
                reco_event_appender.Append<double>(std::numeric_limits<double>::quiet_NaN());
            }
            
            if (particle_valid) {
                reco_event_appender.Append<double>(reco_hadroness);
            } else {
                reco_event_appender.Append<double>(std::numeric_limits<double>::quiet_NaN());
            }
            reco_event_appender.Append<int32_t>(reconstructed_geometry.telescopes.size());
            reco_event_appender.EndRow();
            
            reco_event_count++;
            
            // Flush batch if needed
            if (reco_event_count >= BATCH_SIZE) {
                spdlog::info("Flushing reconstructed event records to database");
                reco_event_appender.Flush();
                reco_event_count = 0;
            }
        }
    }
}

void DatabaseWriter::writeTelescopeData(duckdb::Appender& telescope_appender, 
                                        const ArrayEvent& event, 
                                        const std::string& source_file, 
                                        size_t& tel_data_count, 
                                        bool use_true) 
{
    // 1. 根据 use_true 决定数据源，如果源不存在则直接返回
    if ((use_true && !event.simulation.has_value()) || 
        (!use_true && !event.dl1.has_value())) {
        return;
    }

    // 2. 创建一个 variant 来持有对 DL1Event 或 SimulatedEvent 的引用
    using EventVariant = std::variant<std::reference_wrapper<const DL1Event>, 
                                      std::reference_wrapper<const SimulatedEvent>>;

    EventVariant tel_event = use_true
        ? EventVariant{std::cref(event.simulation.value())}
        : EventVariant{std::cref(event.dl1.value())};

    // 3. 使用 std::visit 来处理 variant，避免代码重复
    std::visit([&](const auto& current_event_wrapper) {
        // .get() 从 reference_wrapper 中获取实际的引用
        const auto& current_event = current_event_wrapper.get();

        // 遍历选定事件源中的所有望远镜
        for (const auto& tel_pair : current_event.tels) {
            int tel_id = tel_pair.first;
            const auto& tel_data = tel_pair.second; // 这是 DL1Camera* 或 SimulatedCamera*

            // 从 tel_data 中直接获取 image_parameters
            const auto& image_params = tel_data->image_parameters;

            // --- 提取公共参数 ---
            const auto& hillas = image_params.hillas;
            const auto& leakage = image_params.leakage;
            const auto& concentration = image_params.concentration;
            const auto& morphology = image_params.morphology;
            const auto& intensity = image_params.intensity;
            const auto& extra = image_params.extra;

            // --- 提取特定参数 (true_impact_parameter) ---
            double true_impact_parameter = std::numeric_limits<double>::quiet_NaN();
            
            // 使用 if constexpr 在编译期判断当前处理的事件类型
            double time_range_10_90 = std::numeric_limits<double>::quiet_NaN();
            if constexpr (std::is_same_v<std::decay_t<decltype(current_event)>, SimulatedEvent>) {
                // 如果是 SimulatedEvent，直接从 tel_data 获取真实 impact parameter
                true_impact_parameter = tel_data->impact_parameter;
                time_range_10_90 = tel_data->time_range_10_90;
            } else {
                // 如果是 DL1Event，需要从 event.simulation 中查找对应的真实 impact parameter
                if (event.simulation.has_value()) {
                    const auto& sim_tels = event.simulation.value().tels;
                    if (sim_tels.contains(tel_id)) {
                        true_impact_parameter = sim_tels.at(tel_id)->impact_parameter;
                        time_range_10_90 = sim_tels.at(tel_id)->time_range_10_90;
                    }
                }
            }

            // --- 提取 DL2 重建参数 (逻辑保持不变) ---
            double reco_impact_parameter = std::numeric_limits<double>::quiet_NaN();
            if (event.dl2.has_value()) {
                const auto& dl2_event = event.dl2.value();
                if (dl2_event.tels.count(tel_id)) {
                    const auto& tel_reco_param = dl2_event.tels.at(tel_id);
                    if (!tel_reco_param.impact_parameters.empty()) {
                        reco_impact_parameter = tel_reco_param.impact_parameters.begin()->second.distance;
                    }
                }
            }

            // --- 数据写入 Appender (逻辑保持不变) ---
            telescope_appender.BeginRow();
            telescope_appender.Append<int32_t>(event.event_id);
            telescope_appender.Append<int32_t>(event.run_id);
            telescope_appender.Append<int32_t>(tel_id);
            telescope_appender.Append<const char*>(source_file.c_str());
            telescope_appender.Append<double>(true_impact_parameter);
            telescope_appender.Append<double>(hillas.intensity);
            telescope_appender.Append<double>(hillas.width);
            telescope_appender.Append<double>(hillas.length);
            telescope_appender.Append<double>(hillas.psi);
            telescope_appender.Append<double>(hillas.x);
            telescope_appender.Append<double>(hillas.y);
            telescope_appender.Append<double>(hillas.skewness);
            telescope_appender.Append<double>(hillas.kurtosis);
            telescope_appender.Append<double>(hillas.r);
            telescope_appender.Append<double>(hillas.phi);
            telescope_appender.Append<double>(leakage.pixels_width_1);
            telescope_appender.Append<double>(leakage.pixels_width_2);
            telescope_appender.Append<double>(leakage.intensity_width_1);
            telescope_appender.Append<double>(leakage.intensity_width_2);
            telescope_appender.Append<double>(concentration.concentration_cog);
            telescope_appender.Append<double>(concentration.concentration_core);
            telescope_appender.Append<double>(concentration.concentration_pixel);
            telescope_appender.Append<int32_t>(morphology.n_pixels);
            telescope_appender.Append<int32_t>(morphology.n_islands);
            telescope_appender.Append<int32_t>(morphology.n_small_islands);
            telescope_appender.Append<int32_t>(morphology.n_medium_islands);
            telescope_appender.Append<int32_t>(morphology.n_large_islands);
            telescope_appender.Append<double>(intensity.intensity_max);
            telescope_appender.Append<double>(intensity.intensity_mean);
            telescope_appender.Append<double>(intensity.intensity_std);
            telescope_appender.Append<double>(intensity.intensity_skewness);
            telescope_appender.Append<double>(intensity.intensity_kurtosis);
            telescope_appender.Append<double>(extra.miss);
            telescope_appender.Append<double>(extra.disp);
            telescope_appender.Append<double>(extra.theta);
            telescope_appender.Append<double>(extra.true_psi);
            telescope_appender.Append<double>(extra.cog_err);
            telescope_appender.Append<double>(extra.beta_err);
            telescope_appender.Append<double>(reco_impact_parameter);
            telescope_appender.Append<double>(time_range_10_90);
            telescope_appender.EndRow();

            // --- 批处理计数和刷新 (逻辑保持不变) ---
            tel_data_count++;
            if (tel_data_count >= BATCH_SIZE) {
                spdlog::info("Flushing telescope records to database");
                telescope_appender.Flush();
                tel_data_count = 0;
            }
        }
    }, tel_event);
}
void DatabaseWriter::clearTables() {
    // Start a transaction for better performance
    auto result = conn_.Query("BEGIN TRANSACTION");
    if (result->HasError()) {
        std::cerr << "Error starting transaction: " << result->GetError() << std::endl;
        return;
    }
    
    // Clear all data from the tables
    result = conn_.Query("DELETE FROM SimulatedShower");
    if (result->HasError()) {
        std::cerr << "Error clearing SimulatedShower table: " << result->GetError() << std::endl;
        return;
    }
    
    result = conn_.Query("DELETE FROM ReconstructedEvent");
    if (result->HasError()) {
        std::cerr << "Error clearing ReconstructedEvent table: " << result->GetError() << std::endl;
        return;
    }
    
    result = conn_.Query("DELETE FROM Telescope");
    if (result->HasError()) {
        std::cerr << "Error clearing Telescope table: " << result->GetError() << std::endl;
        return;
    }
    
    // Commit the transaction
    result = conn_.Query("COMMIT");
    if (result->HasError()) {
        std::cerr << "Error committing transaction: " << result->GetError() << std::endl;
        return;
    }
}