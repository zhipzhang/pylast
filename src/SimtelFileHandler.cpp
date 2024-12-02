#include "SimtelFileHandler.hh"
#include "LACT_hessioxxx/include/mc_atmprof.h"
#include "LACT_hessioxxx/include/mc_tel.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

const std::string ihep_url = "root://eos01.ihep.ac.cn/";
using std::string;
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
    read_runheader();
    read_mcrunheader();
    read_atmosphere();
    read_camera_settings();
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
void SimtelFileHandler::read_camera_settings() {
    spdlog::debug("Read camera settings block");
    while(item_header.type != IO_TYPE_SIMTEL_CAMSETTINGS) {
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read camera settings for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        spdlog::warn("Skip camera settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    spdlog::debug("Read camera settings for tel_id: {}", tel_id);
    if(read_simtel_camsettings(iobuf, &hsdata->camera_set[itel]) < 0) {
        spdlog::error("Failed to read camera settings");
        throw std::runtime_error("Failed to read camera settings");
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
