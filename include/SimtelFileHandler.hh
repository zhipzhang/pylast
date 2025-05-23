/**
 * @file SimtelFileHandler.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief read the simtel file 
 * @version 0.1
 * @date 2024-12-02
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once
#include <string>
#include "LACT_hessioxxx/include/fileopen.h"
#include "LACT_hessioxxx/include/io_hess.h"
#include "LACT_hessioxxx/include/io_basic.h"
#include "LACT_hessioxxx/include/io_history.h"
#include "LACT_hessioxxx/include/mc_atmprof.h"
#include "LACT_hessioxxx/include/initial.h"
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <functional>
#include "spdlog/spdlog.h"

using History_Entry = std::pair<time_t, std::string>;
using History_List = std::vector<History_Entry>;
class SimtelFileHandler {
    friend class SimtelEventSource;
    friend class  SimtelFileHandlerTestSuite;
public:
    enum class BlockType {
        History = IO_TYPE_HISTORY,
        MetaParam = IO_TYPE_METAPARAM,
        RunHeader = IO_TYPE_SIMTEL_RUNHEADER,
        MC_RunHeader = IO_TYPE_SIMTEL_MCRUNHEADER,
        Atmosphere = IO_TYPE_MC_ATMPROF,
        CameraSettings = IO_TYPE_SIMTEL_CAMSETTINGS,
        CameraOrganisation = IO_TYPE_SIMTEL_CAMORGAN,
        PixelSettings = IO_TYPE_SIMTEL_PIXELSET,
        PixelDisabled = IO_TYPE_SIMTEL_PIXELDISABLE,
        CameraSoftwareSettings = IO_TYPE_SIMTEL_CAMSOFTSET,
        PointingCorrections = IO_TYPE_SIMTEL_POINTINGCOR,
        TrackingSettings = IO_TYPE_SIMTEL_TRACKSET,
        TrackingEvent = IO_TYPE_SIMTEL_TRACKEVENT,
        Mc_Shower = IO_TYPE_SIMTEL_MC_SHOWER,
        Mc_Event = IO_TYPE_SIMTEL_MC_EVENT,
        LaserCalibration = IO_TYPE_SIMTEL_LASCAL,
        PixelMonitor = IO_TYPE_SIMTEL_MC_PIXMON,
        TelescopeMonitor = IO_TYPE_SIMTEL_TEL_MONI,
        TrueImage = IO_TYPE_MC_TELARRAY,
        MC_PESUM  = IO_TYPE_SIMTEL_MC_PE_SUM ,
        SimtelEvent = IO_TYPE_SIMTEL_EVENT,
        TEST_BLOCK = 777
    };

    SimtelFileHandler(const std::string& filename);
    SimtelFileHandler() = default;
    ~SimtelFileHandler();
#ifdef DEBUG
public:
#else
private:
#endif
    bool no_more_blocks = false;
    bool have_true_image = false;
    std::string filename = "none";

    // C-Style Pointer here for eventio hsdata
    FILE* input_file = nullptr;
    IO_BUFFER* iobuf = nullptr;
    IO_ITEM_HEADER item_header;
    AllHessData* hsdata = nullptr;
    std::unordered_map<int, int> tel_id_to_index;
    std::unordered_map<std::string, std::string> global_metadata;
    std::unordered_map<int, std::unordered_map<std::string, std::string>> tel_metadata;

    inline std::optional<int> get_tel_index(int tel_id) const;
    inline void initilize_block_handler();

    /**
     * @brief Open the simtel file. Here we provide two ways:
     *        1. open a file by the fileopen provided by hessioxxx
     *        2. open a file by the EventIOWrapper provided by EventIOExtension
     * 
     * @param filename 
     */
    inline void open_file(const std::string& filename);
    /**
     * @brief Keep find_block until we meet the block type specified in block_type
     *        Keep in mind that read_block must be called after this function.
     *        There will be two ways to return from this function:
     *        1. meet the block type specified in block_type
     *        2. meet the end of file, in this case, no_more_blocks will be set to true.
     * @param block_type 
     */
    void read_until_block(BlockType block_type);
    /**
     * @brief Only read the blocks specified in the block_types, for other blocks, skip_block is called.
     *        It should be noted that when we meet the first block type in the block_types, we will read it and return.
     *        for example, if block_types = {BlockType::Mc_Event, BlockType::Mc_Shower},
     *        when we meet the Mc_Event block, we will read it and return, and then we will not read the Mc_Shower block.
     * @param block_types can contain multiple block types
     */
    void only_read_blocks(std::vector<BlockType> block_types);
    bool only_read_mc_event();
    /**
     * @brief Read the simtel file until we meet the SimtelEvent block and read it.
     * 
     * @return true  still have events to read
     * @return false end of file
     */
    bool load_next_event();
    /**
     * @brief read the sim_tel file until we meet the Actually MC_Shower
     * 
     */
    void read_until_event();
    AtmProf* atmprof;
    HistoryContainer history_container;
    MetaParamList metadata_list;
    void find_block();
    void skip_block();
    void read_block();
    void _read_history();
    void _read_metadata();
    void _read_runheader();
    void _read_mcrunheader();
    void _read_atmosphere();
    void _read_camera_settings();
    void _read_camera_organisation();
    void _read_pixel_settings();
    void _read_pixel_disabled();
    void _read_camera_software_settings();
    void _read_pointing_corrections();
    void _read_tracking_settings();
    void _read_telescope_settings();
    void _read_mc_shower();
    void _read_mc_event();
    void _read_pixel_monitor();
    void _read_telescope_monitor();
    void _read_laser_calibration();
    void _read_true_image();
    void _read_mc_pesum();
    void _read_simtel_event();
    void handle_history();
    void handle_metadata();
    void handle_runheader();
    void handle_mcrunheader();
    void handle_atmosphere();
    void handle_camera_settings();
    void handle_camera_organisation();
    void handle_pixel_settings();
    void handle_pixel_disabled();
    void handle_camera_software_settings();
    void handle_pointing_corrections();
    void handle_tracking_settings();
    void handle_telescope_settings();
    void handle_mc_shower();
    void handle_mc_event();
    void handle_pixel_monitor();
    void handle_telescope_monitor();
    void handle_laser_calibration();
    void handle_true_image();
    void handle_simtel_event();
    void handle_mc_pesum();
    std::unordered_map<BlockType, std::function<void()>> block_handler;
    // In case of valgrind, let's clear the hsdata memory here
    void clear_memory();

    template<BlockType block_type>
    void handle_block(std::string_view block_name, std::function<void()> handler = nullptr) {
        [[unlikely]] if(item_header.type != static_cast<unsigned long>(block_type)) {
            spdlog::warn("Skip block type: {}", block_name);
            return;
        }
        if(handler) {
            handler();
        }
    }
    
};
