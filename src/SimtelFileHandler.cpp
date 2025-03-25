#include <optional>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "SimtelFileHandler.hh"
#include "LACT_hessioxxx/include/io_hess.h"
#include "LACT_hessioxxx/include/io_history.h"
#include "LACT_hessioxxx/include/mc_atmprof.h"
#include "LACT_hessioxxx/include/mc_tel.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

#define LOG_SCOPE(message)\
    SPDLOG_DEBUG("Begin {}", message);\
    auto scope_exit = finally([msg = message](){SPDLOG_DEBUG("End {}", msg);});

template <typename Func>
struct Finally {
    Func func;
    ~Finally() { func(); }
};

template <typename Func>
Finally<Func> finally(Func func) { return {func}; }

const std::string ihep_url = "root://eos01.ihep.ac.cn:/";
using std::string;
SimtelFileHandler::SimtelFileHandler(const std::string& filename) : filename(filename) {
    SPDLOG_TRACE("SimtelFileHandler constructor ");
    if((iobuf = allocate_io_buffer(5000000L)) == NULL) {
        throw std::runtime_error("Cannot allocate I/O buffer");
    }
    iobuf->output_file = NULL;
    iobuf->max_length = 1000000000L;
    item_header = {0, 0, 0, 0, 0, 0, 0, 0};
    history_container = {1, NULL, NULL, NULL, 0};
    metadata_list = {-1, NULL};
    atmprof = get_common_atmprof();
    initilize_block_handler();
    open_file(filename);
    SPDLOG_TRACE("End read simtel file");
}

void SimtelFileHandler::initilize_block_handler() {
    block_handler[BlockType::History] = [this](){handle_history();};
    block_handler[BlockType::MetaParam] = [this](){handle_metadata();};
    block_handler[BlockType::RunHeader] = [this](){handle_runheader();};
    block_handler[BlockType::MC_RunHeader] = [this](){handle_mcrunheader();};
    block_handler[BlockType::Atmosphere] = [this](){handle_atmosphere();};
    block_handler[BlockType::CameraSettings] = [this](){handle_camera_settings();};
    block_handler[BlockType::CameraOrganisation] = [this](){handle_camera_organisation();};
    block_handler[BlockType::PixelSettings] = [this](){handle_pixel_settings();};
    block_handler[BlockType::PixelDisabled] = [this](){handle_pixel_disabled();};
    block_handler[BlockType::CameraSoftwareSettings] = [this](){handle_camera_software_settings();};
    block_handler[BlockType::PointingCorrections] = [this](){handle_pointing_corrections();};
    block_handler[BlockType::TrackingSettings] = [this](){handle_tracking_settings();};
    block_handler[BlockType::Mc_Shower] = [this](){handle_mc_shower();};
    block_handler[BlockType::Mc_Event] = [this](){handle_mc_event();};
    block_handler[BlockType::LaserCalibration] = [this](){handle_laser_calibration();};
    block_handler[BlockType::PixelMonitor] = [this](){handle_pixel_monitor();};
    block_handler[BlockType::TelescopeMonitor] = [this](){handle_telescope_monitor();};
    block_handler[BlockType::TrueImage] = [this](){handle_true_image();};
    block_handler[BlockType::MC_PESUM] = [this](){handle_mc_pesum();};
    block_handler[BlockType::SimtelEvent] = [this](){handle_simtel_event();};
}

void SimtelFileHandler::open_file(const std::string& filename) {
    if (filename.substr(0, 4) == "/eos") {
        // If filename starts with "eos", prepend the IHEP URL
        string full_path = ihep_url  + filename;
        spdlog::info("Opening EOS file: {}", full_path);
        input_file = fileopen(full_path.c_str(), "r");
        if(input_file == NULL)
        {
            spdlog::error("Failed to open EOS file: {}", full_path);
            throw std::runtime_error(spdlog::fmt_lib::format("Failed to open EOS file: {}", full_path));
        }
    }
    else
    {
        input_file = fileopen(filename.c_str(), "r");
        if(input_file == NULL)
        {
            spdlog::error("Failed to open file: {}", filename);
            throw std::runtime_error(spdlog::fmt_lib::format("Failed to open local file: {}", filename));
        }
    }
    iobuf->input_file = input_file;
}

std::optional<int> SimtelFileHandler::get_tel_index (int tel_id) const
{
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        return std::nullopt;
    }
    return it->second;
}
void SimtelFileHandler::find_block() {
    if(no_more_blocks) return;
    if(find_io_block(iobuf, &item_header) != 0) {
        SPDLOG_DEBUG("No more blocks");
        no_more_blocks = true;
        return;
    }
}
void SimtelFileHandler::skip_block() {
    if(no_more_blocks) return;
    if(skip_io_block(iobuf, &item_header) != 0) {
        throw std::runtime_error("Failed to skip block");
    }
}
void SimtelFileHandler::read_block() {
    if(no_more_blocks) return;
    if(read_io_block(iobuf, &item_header) != 0) {
        throw std::runtime_error("Failed to read block");
    }
}
/**
 * @brief Read until the block type is the same as the input block type, after call this function,
 * must handle the block manually
 * 
 * @param block_type 
 */
void SimtelFileHandler::read_until_block(BlockType block_type) {
    find_block();
    while(item_header.type != static_cast<unsigned long>(block_type)) {
        if(no_more_blocks) return;
        if(block_handler.find(static_cast<BlockType>(item_header.type)) != block_handler.end())
        {
            read_block();
            block_handler[static_cast<BlockType>(item_header.type)]();
        }
        else 
        {
            spdlog::warn("No handler for block type: {}", item_header.type);
            skip_block();
        }
        find_block();
    }
    return;
}
void SimtelFileHandler::only_read_blocks(std::vector<BlockType> block_types) {
    if(block_types.empty() || no_more_blocks) return;
    find_block();
    while(!no_more_blocks) {
        if(std::find(block_types.begin(), block_types.end(), static_cast<BlockType>(item_header.type)) != block_types.end()) {
            read_block();
            block_handler[static_cast<BlockType>(item_header.type)]();
            if(item_header.type == static_cast<unsigned long>(block_types.front())) {
                return;
            }
        }
        else {
            skip_block();
        }
        find_block();
    }
}
bool SimtelFileHandler::only_read_mc_event() {
    only_read_blocks({BlockType::Mc_Event, BlockType::Mc_Shower, BlockType::RunHeader});
    if(no_more_blocks) return false;
    return true;
}
void SimtelFileHandler::read_until_event() {
    read_until_block(BlockType::Mc_Shower);
    read_block();
    block_handler[BlockType::Mc_Shower](); // handle mc shower block after read_util
}
bool SimtelFileHandler::load_next_event() {
    read_until_block(BlockType::SimtelEvent);
    read_block();
    if(no_more_blocks) return false;
    block_handler[BlockType::SimtelEvent](); // handle simtel event block after read_util
    return true;
}
void SimtelFileHandler::_read_history() {
    LOG_SCOPE("handle history block")
    // bug in LACT_hessioxxx have been fixed 
    if(read_history(iobuf, &history_container) != 0) {
        spdlog::error("Failed to read history");
        throw std::runtime_error("Failed to read history");
    }
}
void SimtelFileHandler::_read_metadata() {
    LOG_SCOPE("handle metadata block")
    if(read_metaparam(iobuf, &metadata_list) != 0) {
        spdlog::error("Failed to read metadata");
        throw std::runtime_error("Failed to read metadata");
    }
    if(metadata_list.ident == -1) {
        SPDLOG_DEBUG("Read global metadata");
        // global metadata
        while(metadata_list.first) {
                global_metadata[metadata_list.first->name] = metadata_list.first->value;
            metadata_list.first = metadata_list.first->next;
        }
    }
    else {
        SPDLOG_DEBUG("Read tel metadata for tel_id: {}", metadata_list.ident);
        while(metadata_list.first) {
            tel_metadata[metadata_list.ident][metadata_list.first->name] = metadata_list.first->value;
            metadata_list.first = metadata_list.first->next;
        }
    }
}
void SimtelFileHandler::_read_runheader() {
    LOG_SCOPE("handle runheader block")
    if(hsdata == nullptr) {
        hsdata = (AllHessData*)calloc(1, sizeof(AllHessData));
        if(hsdata == NULL) {
            throw std::runtime_error("Cannot allocate memory for hsdata");
        }
    }
    else {
        for(auto itel = 0; itel < hsdata->run_header.ntel; itel++) {
            if(hsdata->event.teldata[itel].raw != NULL) {
                free(hsdata->event.teldata[itel].raw);
            }
            if(hsdata->event.teldata[itel].pixtm != NULL) {
                free(hsdata->event.teldata[itel].pixtm);
            }
            if(hsdata->event.teldata[itel].img != NULL) {
                free(hsdata->event.teldata[itel].img);
            }
            
        }
        memset(hsdata, 0, sizeof(AllHessData));
    }
    if(read_simtel_runheader(iobuf, &hsdata->run_header) < 0) {
        spdlog::error("Failed to read runheader");
        throw std::runtime_error("Failed to read runheader");
    }
        for(auto itel = 0; itel < hsdata->run_header.ntel; itel++)
    {
        int tel_id = hsdata->run_header.tel_id[itel];
        spdlog::info("Initialize telescope id: {} for itel: {}", tel_id, itel);
        tel_id_to_index[tel_id] = itel;
        hsdata->camera_set[itel].tel_id = tel_id;
        hsdata->camera_org[itel].tel_id = tel_id;
        hsdata->pixel_disabled[itel].tel_id = tel_id;
        hsdata->pixel_set[itel].tel_id  = tel_id;
        hsdata->cam_soft_set[itel].tel_id = tel_id;
        hsdata->tracking_set[itel].tel_id = tel_id;
        hsdata->point_cor[itel].tel_id = tel_id;
        hsdata->event.num_tel++;
        hsdata->event.teldata[itel].tel_id = tel_id;
        hsdata->event.trackdata[itel].tel_id = tel_id;
        hsdata->event.teldata[itel].raw = (AdcData*) calloc(1, sizeof(AdcData));
        if(hsdata->event.teldata[itel].raw == NULL)
        {
            spdlog::error("Failed to allocate memory for raw adc data");
            throw std::runtime_error("Failed to allocate memory for raw adc data");
        }
        hsdata->event.teldata[itel].raw->tel_id = tel_id;
        hsdata->event.teldata[itel].pixtm = (PixelTiming*) calloc(1, sizeof(PixelTiming));
        if(hsdata->event.teldata[itel].pixtm == NULL)
        {
            spdlog::error("Failed to allocate memory for pixel timing data");
            throw std::runtime_error("Failed to allocate memory for pixel timing data");
        }
        hsdata->event.teldata[itel].pixtm->tel_id = tel_id;
        hsdata->event.teldata[itel].img = (ImgData*) calloc(2, sizeof(ImgData));
        if(hsdata->event.teldata[itel].img == NULL)
        {
            spdlog::error("Failed to allocate memory for image data");
            throw std::runtime_error("Failed to allocate memory for image data");
        }
        hsdata->event.teldata[itel].img->tel_id = tel_id;
        hsdata->event.teldata[itel].max_image_sets = 1;
        hsdata->event.teldata[itel].img[0].tel_id = tel_id;
        hsdata->event.teldata[itel].img[1].tel_id = tel_id;
        hsdata->tel_moni[itel].tel_id = tel_id;
        hsdata->tel_lascal[itel].tel_id = tel_id;

    }
}
void SimtelFileHandler::_read_mcrunheader() {
    LOG_SCOPE("handle mcrunheader block");
    if(read_simtel_mcrunheader(iobuf, &hsdata->mc_run_header) < 0) {
        spdlog::error("Failed to read mcrunheader");
        throw std::runtime_error("Failed to read mcrunheader");
    }
}
void SimtelFileHandler::_read_atmosphere() {
    LOG_SCOPE("handle atmosphere block");
    if(read_atmprof(iobuf, atmprof) != 0) {
        spdlog::error("Failed to read atmosphere");
        throw std::runtime_error("Failed to read atmosphere");
    }
}
/**
 * @brief Camera settings is the first item in telescope settings block, we need handle it before others
 * 
 */
void SimtelFileHandler::_read_camera_settings() {
    LOG_SCOPE("Read camera settings block");
    // For camera setting, we need read block before call read_simtel_camsettings
    int tel_id = item_header.ident;
    spdlog::debug("Read camera settings for tel_id: {}", tel_id);
    auto it = get_tel_index(tel_id);
    if(!it) {
        SPDLOG_WARN("Skip telescope settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it.value();
    //memset(&hsdata->camera_set[itel], 0, sizeof(CameraSettings));
    if(read_simtel_camsettings(iobuf, &hsdata->camera_set[itel]) < 0) {
        spdlog::error("Failed to read camera settings");
        throw std::runtime_error("Failed to read camera settings");
    }
}
void SimtelFileHandler::_read_camera_organisation() {
    LOG_SCOPE("Read camera organisation block");
    int tel_id = item_header.ident;
    spdlog::debug("Read camera organisation for tel_id: {}", tel_id);
    auto it = get_tel_index(tel_id);
    if(!it) {
        SPDLOG_WARN("Skip camera organisation for tel_id: {}", tel_id);
        return;
    }
    int itel = it.value();
    if(read_simtel_camorgan(iobuf, &hsdata->camera_org[itel]) < 0) {
        spdlog::error("Failed to read camera organisation");
        throw std::runtime_error("Failed to read camera organisation");
    }
}
void SimtelFileHandler::_read_pixel_settings() {
    LOG_SCOPE("Read pixel settings block");
    int tel_id = item_header.ident;
    spdlog::debug("Read pixel settings for tel_id: {}", tel_id);
    auto it = get_tel_index(tel_id);
    if(!it) {
        SPDLOG_WARN("Skip pixel settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it.value();
    if(read_simtel_pixelset(iobuf, &hsdata->pixel_set[itel]) < 0) {
        spdlog::error("Failed to read pixel settings");
        throw std::runtime_error("Failed to read pixel settings");
    }   
}
void SimtelFileHandler::_read_pixel_disabled() {
    LOG_SCOPE("Read pixel disabled block");
    int tel_id = item_header.ident;
    spdlog::debug("Read pixel disabled for tel_id: {}", tel_id);
    auto it = get_tel_index(tel_id);
    if(!it) {
        SPDLOG_WARN("Skip pixel disabled for tel_id: {}", tel_id);
        return;
    }
    int itel = it.value();
    if(read_simtel_pixeldis(iobuf, &hsdata->pixel_disabled[itel]) < 0) {
        spdlog::error("Failed to read pixel disabled");
        throw std::runtime_error("Failed to read pixel disabled");
    }
}

void SimtelFileHandler::_read_camera_software_settings() {
    LOG_SCOPE("Read camera software settings block");
    int tel_id = item_header.ident;
    spdlog::debug("Read camera software settings for tel_id: {}", tel_id);
    auto it = get_tel_index(tel_id);
    if(!it) {
        SPDLOG_WARN("Skip camera software settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it.value();
    if(read_simtel_camsoftset(iobuf, &hsdata->cam_soft_set[itel]) < 0) {
        spdlog::error("Failed to read camera software settings");
        throw std::runtime_error("Failed to read camera software settings");
    }
}

void SimtelFileHandler::_read_pointing_corrections() {
    LOG_SCOPE("Read pointing corrections block");
    int tel_id = item_header.ident;
    spdlog::debug("Read pointing corrections for tel_id: {}", tel_id);
    auto it = get_tel_index(tel_id);
    if(!it) {
        SPDLOG_WARN("Skip pointing corrections for tel_id: {}", tel_id);
        return;
    }
    int itel = it.value();
    if(read_simtel_pointingcor(iobuf, &hsdata->point_cor[itel]) < 0) {
        spdlog::error("Failed to read pointing corrections");
        throw std::runtime_error("Failed to read pointing corrections");
    }
}

void SimtelFileHandler::_read_tracking_settings() {
    LOG_SCOPE("Read tracking settings block");
    int tel_id = item_header.ident;
    spdlog::debug("Read tracking settings for tel_id: {}", tel_id);
    auto it = get_tel_index(tel_id);
    if(!it) {
        SPDLOG_WARN("Skip tracking settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it.value();
    if(read_simtel_trackset(iobuf, &hsdata->tracking_set[itel]) < 0) {
        spdlog::error("Failed to read tracking settings");
        throw std::runtime_error("Failed to read tracking settings");
    }
}

void SimtelFileHandler::_read_mc_shower() {
    LOG_SCOPE("Read mc shower block");
    int run_id = item_header.ident;
    spdlog::debug("Read mc shower for run_id: {}", run_id);
    if(read_simtel_mc_shower(iobuf, &hsdata->mc_shower) != 0) {
        spdlog::error("Failed to read mc shower");
        throw std::runtime_error("Failed to read mc shower");
    }
}

void SimtelFileHandler::_read_mc_event() {
    int event_id = item_header.ident;
    spdlog::debug("Read mc event for event_id: {}", event_id);
    if(read_simtel_mc_event(iobuf, &hsdata->mc_event) != 0) {
        spdlog::error("Failed to read mc event");
        throw std::runtime_error("Failed to read mc event");
    }
}
void SimtelFileHandler::_read_pixel_monitor() {
    LOG_SCOPE("Read pixel monitor block");
    int tel_id = item_header.ident;
    spdlog::debug("Read pixel monitor for tel_id: {}", tel_id);
    auto it = get_tel_index(tel_id);
    if(!it) {
        SPDLOG_WARN("Skip pixel monitor for tel_id: {}", tel_id);
        return;
    }
    int itel = it.value();
    if(read_simtel_mc_pixel_moni(iobuf, &hsdata->mcpixmon[itel]) != 0) {
        spdlog::error("Failed to read pixel monitor");
        throw std::runtime_error("Failed to read pixel monitor");
    }
}
void SimtelFileHandler::_read_telescope_monitor() {
    LOG_SCOPE("Read telescope monitor block");
    int tel_id = (item_header.ident & 0xff) | 
                     ((item_header.ident & 0x3f000000) >> 16); 
    spdlog::debug("Read telescope monitor for tel_id: {}", tel_id);
    auto it = get_tel_index(tel_id);
    if(!it) {
        SPDLOG_WARN("Skip telescope monitor for tel_id: {}", tel_id);
        return;
    }
    int itel = it.value();
    if(read_simtel_tel_monitor(iobuf, &hsdata->tel_moni[itel]) != 0) {
        spdlog::error("Failed to read telescope monitor");
        throw std::runtime_error("Failed to read telescope monitor");
    }
}
void SimtelFileHandler::_read_true_image() {
    if(!have_true_image)
    {
        have_true_image = true;
    }
    LOG_SCOPE("Read true image block");
    int event_id = item_header.ident;
    spdlog::debug("Read true image for event_id: {}", event_id);
    if(read_simtel_mc_phot(iobuf, &hsdata->mc_event) != 0) {
        spdlog::error("Failed to read true image");
        throw std::runtime_error("Failed to read true image");
    }
}
void SimtelFileHandler::_read_laser_calibration() {
    LOG_SCOPE("Read laser calibration block");
    int tel_id = item_header.ident;
    spdlog::debug("Read laser calibration for tel_id: {}", tel_id);
    auto it = get_tel_index(tel_id);
    if(!it) {
        SPDLOG_WARN("Skip laser calibration for tel_id: {}", tel_id);
        return;
    }
    int itel = it.value();
    if(read_simtel_laser_calib(iobuf, &hsdata->tel_lascal[itel]) != 0) {
        spdlog::error("Failed to read laser calibration");
        throw std::runtime_error("Failed to read laser calibration");
    }
}
void SimtelFileHandler::_read_mc_pesum() {
    LOG_SCOPE("Read mc pesum block");
    int event_id = item_header.ident;
    spdlog::debug("Read mc pesum for event_id: {}", event_id);
    if(read_simtel_mc_pe_sum(iobuf, &hsdata->mc_event.mc_pesum) != 0) {
        spdlog::error("Failed to read mc pesum");
        throw std::runtime_error("Failed to read mc pesum");
    }
}
void SimtelFileHandler::_read_simtel_event() {
    LOG_SCOPE("Read simtel event block");
    int event_id = item_header.ident;
    spdlog::debug("Read simtel event for event_id: {}", event_id);
    // One should known that all telescopes are read out here, 
    // so we should skip afterwards
    if(read_simtel_event(iobuf, &hsdata->event, -1) != 0) {
        spdlog::error("Failed to read simtel event");
        throw std::runtime_error("Failed to read simtel event");
    }
}
void SimtelFileHandler::handle_history() {
    handle_block<BlockType::History>("history",[this]() {_read_history();});
}
void SimtelFileHandler::handle_metadata() {
    handle_block<BlockType::MetaParam>("metadata",[this]() {_read_metadata();});
}
void SimtelFileHandler::handle_runheader() {
    handle_block<BlockType::RunHeader>("runheader",[this]() {_read_runheader();});
}
void SimtelFileHandler::handle_mcrunheader() {
    handle_block<BlockType::MC_RunHeader>("mcrunheader",[this]() {_read_mcrunheader();});
}
void SimtelFileHandler::handle_atmosphere() {
    handle_block<BlockType::Atmosphere>("atmosphere",[this]() {_read_atmosphere();});
}
void SimtelFileHandler::handle_camera_settings() {
    handle_block<BlockType::CameraSettings>("camera_settings",[this]() {_read_camera_settings();});
}
void SimtelFileHandler::handle_camera_organisation() {
    handle_block<BlockType::CameraOrganisation>("camera_organisation",[this]() {_read_camera_organisation();});
}
void SimtelFileHandler::handle_pixel_settings() {
    handle_block<BlockType::PixelSettings>("pixel_settings",[this]() {_read_pixel_settings();});
}
void SimtelFileHandler::handle_pixel_disabled() {
    handle_block<BlockType::PixelDisabled>("pixel_disabled",[this]() {_read_pixel_disabled();});
}
void SimtelFileHandler::handle_camera_software_settings() {
    handle_block<BlockType::CameraSoftwareSettings>("camera_software_settings",[this]() {_read_camera_software_settings();});
}
void SimtelFileHandler::handle_pointing_corrections() {
    handle_block<BlockType::PointingCorrections>("pointing_corrections",[this]() {_read_pointing_corrections();});
}
void SimtelFileHandler::handle_tracking_settings() {
    handle_block<BlockType::TrackingSettings>("tracking_settings",[this]() {_read_tracking_settings();});
}
void SimtelFileHandler::handle_mc_shower() {
    handle_block<BlockType::Mc_Shower>("mc_shower",[this]() {_read_mc_shower();});
}
void SimtelFileHandler::handle_mc_event() {
    handle_block<BlockType::Mc_Event>("mc_event",[this]() {_read_mc_event();});
}
void SimtelFileHandler::handle_pixel_monitor() {
    handle_block<BlockType::PixelMonitor>("pixel_monitor",[this]() {_read_pixel_monitor();});
}
void SimtelFileHandler::handle_telescope_monitor() {
    handle_block<BlockType::TelescopeMonitor>("telescope_monitor",[this]() {_read_telescope_monitor();});
}
void SimtelFileHandler::handle_laser_calibration() {
    handle_block<BlockType::LaserCalibration>("laser_calibration",[this]() {_read_laser_calibration();});
}
void SimtelFileHandler::handle_true_image() {
    handle_block<BlockType::TrueImage>("true_image",[this]() {_read_true_image();});
}
void SimtelFileHandler::handle_mc_pesum() {
    handle_block<BlockType::MC_PESUM>("mc_pesum",[this]() {_read_mc_pesum();});
}
void SimtelFileHandler::handle_simtel_event() {
    handle_block<BlockType::SimtelEvent>("simtel_event",[this]() {_read_simtel_event();});
}
/*
void SimtelFileHandler::load_next_event() {
    read_mc_shower();
    if(no_more_blocks) return;
    read_mc_event();
    read_true_image();
}
*/
SimtelFileHandler::~SimtelFileHandler() {
    if(iobuf != NULL) {
        free_io_buffer(iobuf);
    }
    if(input_file != NULL) {
        fileclose(input_file);
    }
    if(hsdata != NULL) {
        clear_memory();
    }
}
void SimtelFileHandler::clear_memory() {
    if(hsdata != NULL) {
        for(auto itel = 0; itel < hsdata->run_header.ntel; itel++) {
            if(hsdata->event.teldata[itel].raw != NULL) {
                free(hsdata->event.teldata[itel].raw);
            }
            if(hsdata->event.teldata[itel].pixtm != NULL) {
                free(hsdata->event.teldata[itel].pixtm);
            }
            if(hsdata->event.teldata[itel].img != NULL) {
                free(hsdata->event.teldata[itel].img);
            }
            if(hsdata->mc_event.mc_pe_list[itel].atimes != NULL) {
                free(hsdata->mc_event.mc_pe_list[itel].atimes);
            }
            if(hsdata->mc_event.mc_pe_list[itel].amplitudes != NULL) {
                free(hsdata->mc_event.mc_pe_list[itel].amplitudes);
            }
        }
        free(hsdata);
    }
}
