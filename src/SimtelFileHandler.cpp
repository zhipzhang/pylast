#include "SimtelFileHandler.hh"
#include "LACT_hessioxxx/include/io_history.h"
#include "LACT_hessioxxx/include/mc_atmprof.h"
#include "LACT_hessioxxx/include/mc_tel.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

const std::string ihep_url = "root://eos01.ihep.ac.cn/";
using std::string;
HistoryContainer SimtelFileHandler::history_container = {1, NULL, NULL, NULL, 0};
MetaParamList SimtelFileHandler::metadata_list;
SimtelFileHandler::SimtelFileHandler(const std::string& filename, std::vector<int> subarray) : filename(filename), allowed_tels(subarray) {
    if((iobuf = allocate_io_buffer(5000000L)) == NULL) {
        throw std::runtime_error("Cannot allocate I/O buffer");
    }
    iobuf->output_file = NULL;
    iobuf->max_length = 1000000000L;
    hsdata = (AllHessData*)calloc(1, sizeof(AllHessData));
    if(hsdata == NULL) {
        throw std::runtime_error("Cannot allocate memory for hsdata");
    }
    open_file(filename);
    _read_history();
    read_metadata();
    read_runheader();
    read_mcrunheader();
    read_atmosphere();
    read_telescope_settings();
}
void SimtelFileHandler::open_file(const std::string& filename) {
    if (filename.substr(0, 4) == "/eos") {
        // If filename starts with "eos", prepend the IHEP URL
        string full_path = ihep_url  + filename;
        spdlog::info("Opening EOS file: {}", full_path);
        input_file = fileopen(full_path.c_str(), "rb");
        if(input_file == NULL)
        {
            spdlog::error("Failed to open EOS file: {}", full_path);
            throw std::runtime_error(spdlog::fmt_lib::format("Failed to open EOS file: {}", full_path));
        }
        return;
    }
    else
    {
        input_file = fileopen(filename.c_str(), "rb");
        if(input_file == NULL)
        {
            spdlog::error("Failed to open file: {}", filename);
            throw std::runtime_error(spdlog::fmt_lib::format("Failed to open local file: {}", filename));
        }
    }
    iobuf->input_file = input_file;
}
void SimtelFileHandler::read_block() {
    if(find_io_block(iobuf, &item_header) != 0) {
        spdlog::error("Failed to find IO block");
        throw std::runtime_error("Failed to find IO block");
    }
    if(read_io_block(iobuf, &item_header) != 0) {
        spdlog::error("Failed to read IO block");
        throw std::runtime_error("Failed to read IO block");
    }
}
void SimtelFileHandler::_read_history() {
    spdlog::debug("Read history block");
    read_block();
    // history is the first item in the file
    spdlog::debug("Read history block type: {}", item_header.type);
    assert(item_header.type == IO_TYPE_HISTORY);

    
    // bug in LACT_hessioxxx have been fixed 
    if(read_history(iobuf, &history_container) != 0) {
        spdlog::error("Failed to read history");
        throw std::runtime_error("Failed to read history");
    }
    spdlog::debug("End read history block");
}
void SimtelFileHandler::read_metadata() {
    spdlog::debug("Read metadata block");
    read_block();
    while(item_header.type == IO_TYPE_METAPARAM) {
        if(read_metaparam(iobuf, &metadata_list) != 0) {
            spdlog::error("Failed to read metadata");
            throw std::runtime_error("Failed to read metadata");
        }
        if(metadata_list.ident == -1) {
            spdlog::debug("Read global metadata");
            // global metadata
            while(metadata_list.first) {
                global_metadata[metadata_list.first->name] = metadata_list.first->value;
                metadata_list.first = metadata_list.first->next;
            }
        }
        else {
            spdlog::debug("Read tel metadata for tel_id: {}", metadata_list.ident);
            while(metadata_list.first) {
                tel_metadata[metadata_list.ident][metadata_list.first->name] = metadata_list.first->value;
                metadata_list.first = metadata_list.first->next;
            }
        }
        read_block();
    }
    spdlog::debug("End read metadata block");
}
void SimtelFileHandler::read_telescope_settings() {
    spdlog::debug("Begin read telescope settings block");
    read_block();
    while(item_header.type == IO_TYPE_SIMTEL_CAMSETTINGS) 
    {
        read_camera_settings();
        read_camera_organisation();
        read_pixel_settings();
        read_pixel_disabled();
        read_camera_software_settings();
        read_tracking_settings();
        read_pointing_corrections();
        read_block();
    }
    spdlog::debug("End read telescope settings block");
}
void SimtelFileHandler::read_runheader() {
    spdlog::debug("Read runheader block");
    while(item_header.type != IO_TYPE_SIMTEL_RUNHEADER) {
        read_block();
    }
    if(read_simtel_runheader(iobuf, &hsdata->run_header) < 0) {
        spdlog::error("Failed to read runheader");
        throw std::runtime_error("Failed to read runheader");
    }
        for(auto itel = 0; itel < hsdata->run_header.ntel; itel++)
    {
        if(!is_subarray_selected(hsdata->run_header.tel_id[itel])) continue;
        int tel_id = hsdata->run_header.tel_id[itel];
        spdlog::info("Initialize telescope id: {}", tel_id);
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
        hsdata->event.teldata[itel].img = (ImgData*) calloc(1, sizeof(ImgData));
        if(hsdata->event.teldata[itel].img == NULL)
        {
            spdlog::error("Failed to allocate memory for image data");
            throw std::runtime_error("Failed to allocate memory for image data");
        }
        hsdata->event.teldata[itel].img->tel_id = tel_id;
        hsdata->event.teldata[itel].max_image_sets = 0;
        hsdata->event.teldata[itel].img[0].tel_id = tel_id;
        hsdata->event.teldata[itel].img[1].tel_id = tel_id;
        hsdata->tel_moni[itel].tel_id = tel_id;
        hsdata->tel_lascal[itel].tel_id = tel_id;

    }

}
void SimtelFileHandler::read_mcrunheader() {
    spdlog::debug("Read mcrunheader block");
    while(item_header.type != IO_TYPE_SIMTEL_MCRUNHEADER) {
        read_block();
    }
    if(read_simtel_mcrunheader(iobuf, &hsdata->mc_run_header) < 0) {
        spdlog::error("Failed to read mcrunheader");
        throw std::runtime_error("Failed to read mcrunheader");
    }
}
void SimtelFileHandler::read_atmosphere() {
    spdlog::debug("Read atmosphere block");
    atmprof = get_common_atmprof();
    while(item_header.type != IO_TYPE_MC_ATMPROF) {
        read_block();
    }
    if(read_atmprof(iobuf, atmprof) != 0) {
        spdlog::error("Failed to read atmosphere");
        throw std::runtime_error("Failed to read atmosphere");
    }
}
/**
 * @brief Camera settings is the first item in telescope settings block, we need handle it before others
 * 
 */
void SimtelFileHandler::read_camera_settings() {
    spdlog::debug("Read camera settings block");
    // For camera setting, we need read block before call read_simtel_camsettings
    assert(item_header.type == IO_TYPE_SIMTEL_CAMSETTINGS);
    int tel_id = item_header.ident;
    spdlog::debug("Read camera settings for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        spdlog::warn("Skip camera settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_camsettings(iobuf, &hsdata->camera_set[itel]) < 0) {
        spdlog::error("Failed to read camera settings");
        throw std::runtime_error("Failed to read camera settings");
    }

}
void SimtelFileHandler::read_camera_organisation() {
    spdlog::debug("Read camera organisation block");
    while(item_header.type != IO_TYPE_SIMTEL_CAMORGAN) {
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read camera organisation for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        spdlog::warn("Skip camera organisation for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_camorgan(iobuf, &hsdata->camera_org[itel]) < 0) {
        spdlog::error("Failed to read camera organisation");
        throw std::runtime_error("Failed to read camera organisation");
    }
}
void SimtelFileHandler::read_pixel_settings() {
    spdlog::debug("Read pixel settings block");
    while(item_header.type != IO_TYPE_SIMTEL_PIXELSET) {
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read pixel settings for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        spdlog::warn("Skip pixel settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_pixelset(iobuf, &hsdata->pixel_set[itel]) < 0) {
        spdlog::error("Failed to read pixel settings");
        throw std::runtime_error("Failed to read pixel settings");
    }   
}
void SimtelFileHandler::read_pixel_disabled() {
    spdlog::debug("Read pixel disabled block");
    while(item_header.type != IO_TYPE_SIMTEL_PIXELDISABLE) {
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read pixel disabled for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        spdlog::warn("Skip pixel disabled for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_pixeldis(iobuf, &hsdata->pixel_disabled[itel]) < 0) {
        spdlog::error("Failed to read pixel disabled");
        throw std::runtime_error("Failed to read pixel disabled");
    }
}

void SimtelFileHandler::read_camera_software_settings() {
    spdlog::debug("Read camera software settings block");
    while(item_header.type != IO_TYPE_SIMTEL_CAMSOFTSET) {
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read camera software settings for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        spdlog::warn("Skip camera software settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_camsoftset(iobuf, &hsdata->cam_soft_set[itel]) < 0) {
        spdlog::error("Failed to read camera software settings");
        throw std::runtime_error("Failed to read camera software settings");
    }
}

void SimtelFileHandler::read_pointing_corrections() {
    spdlog::debug("Read pointing corrections block");
    while(item_header.type != IO_TYPE_SIMTEL_POINTINGCOR) {
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read pointing corrections for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        spdlog::warn("Skip pointing corrections for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_pointingcor(iobuf, &hsdata->point_cor[itel]) < 0) {
        spdlog::error("Failed to read pointing corrections");
        throw std::runtime_error("Failed to read pointing corrections");
    }
}

void SimtelFileHandler::read_tracking_settings() {
    spdlog::debug("Read tracking settings block");
    while(item_header.type != IO_TYPE_SIMTEL_TRACKSET) {
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read tracking settings for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        spdlog::warn("Skip tracking settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_trackset(iobuf, &hsdata->tracking_set[itel]) < 0) {
        spdlog::error("Failed to read tracking settings");
        throw std::runtime_error("Failed to read tracking settings");
    }
}
bool SimtelFileHandler::is_subarray_selected(int tel_id) {
    if(allowed_tels.empty()) return true;
    return std::find(allowed_tels.begin(), allowed_tels.end(), tel_id) != allowed_tels.end();
}
SimtelFileHandler::~SimtelFileHandler() {
    if(iobuf != NULL) {
        free_io_buffer(iobuf);
    }
    if(input_file != NULL) {
        fileclose(input_file);
    }
    if(hsdata != NULL) {
        free(hsdata);
    }
}
