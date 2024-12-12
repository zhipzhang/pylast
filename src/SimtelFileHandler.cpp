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

const std::string ihep_url = "root://eos01.ihep.ac.cn/";
using std::string;
SimtelFileHandler::SimtelFileHandler(const std::string& filename, std::vector<int> subarray) : filename(filename), allowed_tels(subarray) {
    SPDLOG_TRACE("SimtelFileHandler constructor ");
    if((iobuf = allocate_io_buffer(5000000L)) == NULL) {
        throw std::runtime_error("Cannot allocate I/O buffer");
    }
    iobuf->output_file = NULL;
    iobuf->max_length = 1000000000L;
    hsdata = (AllHessData*)calloc(1, sizeof(AllHessData));
    if(hsdata == NULL) {
        throw std::runtime_error("Cannot allocate memory for hsdata");
    }
    history_container = {1, NULL, NULL, NULL, 0};
    metadata_list = {-1, NULL};
    atmprof = get_common_atmprof();
    initilize_block_handler();
    open_file(filename);
    read_until_event();
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

std::optional<int> SimtelFileHandler::get_tel_index (int tel_id) const
{
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        return std::nullopt;
    }
    return it->second;
}
void SimtelFileHandler::read_block() {
    if(find_io_block(iobuf, &item_header) != 0) {
        SPDLOG_DEBUG("No more blocks");
        no_more_blocks = true;
        return;
    }
    if(read_io_block(iobuf, &item_header) != 0) {
        spdlog::error("Failed to read IO block");
        throw std::runtime_error("Failed to read IO block");
    }
}
void SimtelFileHandler::read_until_block(BlockType block_type) {
    while(item_header.type != static_cast<unsigned long>(block_type)) {
        if(no_more_blocks) return;
        read_block();
        if(block_handler.find(static_cast<BlockType>(item_header.type)) != block_handler.end())
        {
            block_handler[static_cast<BlockType>(item_header.type)]();
        }
        else 
        {
            spdlog::warn("No handler for block type: {}", item_header.type);
        }
    }
}
void SimtelFileHandler::read_until_event() {
    read_until_block(BlockType::Mc_Shower);
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
    if(read_simtel_runheader(iobuf, &hsdata->run_header) < 0) {
        spdlog::error("Failed to read runheader");
        throw std::runtime_error("Failed to read runheader");
    }
        for(auto itel = 0; itel < hsdata->run_header.ntel; itel++)
    {
        if(!is_subarray_selected(hsdata->run_header.tel_id[itel])) 
        {
            SPDLOG_DEBUG("Skip telescope id: {} in runheader", hsdata->run_header.tel_id[itel]);
            continue;
        }
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
bool SimtelFileHandler::is_subarray_selected(int tel_id) {
    if(allowed_tels.empty()) return true;
    return std::find(allowed_tels.begin(), allowed_tels.end(), tel_id) != allowed_tels.end();
}

void SimtelFileHandler::_read_mc_shower() {
    LOG_SCOPE("Read mc shower block");
    // Cause we may need skip some blocks before read mc shower
    read_until_block(BlockType::Mc_Shower);
    read_block();
    // Use warnning before finishing reading the events block
    //spdlog::warn("Skip block type: {}", item_header.type);
    int run_id = item_header.ident;
    spdlog::debug("Read mc shower for run_id: {}", run_id);
    if(read_simtel_mc_shower(iobuf, &hsdata->mc_shower) != 0) {
        spdlog::error("Failed to read mc shower");
        throw std::runtime_error("Failed to read mc shower");
    }
    read_block();
}

void SimtelFileHandler::_read_mc_event() {
    assert(item_header.type == IO_TYPE_SIMTEL_MC_EVENT);
    int event_id = item_header.ident;
    spdlog::debug("Read mc event for event_id: {}", event_id);
    if(read_simtel_mc_event(iobuf, &hsdata->mc_event) != 0) {
        spdlog::error("Failed to read mc event");
        throw std::runtime_error("Failed to read mc event");
    }
    read_block();
}
/*
void SimtelFileHandler::_read_true_image() {
    if(item_header.type == IO_TYPE_MC_TELARRAY)
    {
        if(!have_true_image)
        {
            have_true_image = true;
        }
        spdlog::debug("Reading true image for tel_id: {}", item_header.ident + 1);
        if(_read_simtel_mc_phot(iobuf, &hsdata->mc_event) < 0) {
            spdlog::error("Failed to read true image");
            throw std::runtime_error("Failed to read true image");
        }
    }
    else{
        spdlog::debug("No true image block in the file");
        return;
    }
}
*/
int SimtelFileHandler::_read_simtel_mc_phot(IO_BUFFER* iobuf, MCEvent* mce) {
     
   int iarray=0, itel=0, itel_pe=0, tel_id=0, jtel=0, type, nbunches=0, max_bunches=0, flags=0;
   int npe=0, pixels=0, max_npe=0;
   int rc;
   double photons=0.;
   IO_ITEM_HEADER item_header;
   if ( (rc = begin_read_tel_array(iobuf, &item_header, &iarray)) < 0 )
      return rc;
   while ( (type = next_subitem_type(iobuf)) > 0 )
   {
      switch (type)
      {
         case IO_TYPE_MC_PHOTONS:
            /* The purpose of this first call to read_tel_photons is only
               to retrieve the array and telescope numbers (the original offset
               number without ignored telescopes, basically telescope ID minus one), etc. */
            /* With a NULL pointer argument, we expect rc = -10 */
            rc = read_tel_photons(iobuf, 0, &iarray, &itel_pe, &photons,
                  NULL, &nbunches);
            if ( rc != -10 )
            {
               get_item_end(iobuf,&item_header);
               return -1;
            }
            tel_id = itel_pe + 1;
            itel = find_tel_idx(tel_id);
            if ( itel < 0 || itel >= H_MAX_TEL )
            {
               Warning("Invalid telescope number in MC photons");
               get_item_end(iobuf,&item_header);
               return -1;
            }
            if ( nbunches > mce->mc_photons[itel].max_bunches || 
                 (nbunches < mce->mc_photons[itel].max_bunches/4 &&
                 mce->mc_photons[itel].max_bunches > 10000) ||
                 mce->mc_photons[itel].bunches == NULL )
            {
               if ( mce->mc_photons[itel].bunches != NULL )
                  free(mce->mc_photons[itel].bunches);
               if ( (mce->mc_photons[itel].bunches = (struct bunch *)
                    calloc(nbunches,sizeof(struct bunch))) == NULL )
               {
                  mce->mc_photons[itel].max_bunches = 0;
                  get_item_end(iobuf,&item_header);
                  return -4;
               }
               mce->mc_photons[itel].max_bunches = max_bunches = nbunches;
            }
            else
               max_bunches = mce->mc_photons[itel].max_bunches;

            /* Now really get the photon bunches */
            rc = read_tel_photons(iobuf, max_bunches, &iarray, &jtel, 
               &photons, mce->mc_photons[itel].bunches, &nbunches);

            if ( rc < 0 )
            {
               mce->mc_photons[itel].nbunches = 0;
               get_item_end(iobuf,&item_header);
               return rc;
            }
            else
               mce->mc_photons[itel].nbunches = nbunches;

            if ( jtel != itel )
            {
               Warning("Inconsistent telescope number for MC photons");
               get_item_end(iobuf,&item_header);
               return -5;
            }
            break;
         case IO_TYPE_MC_PE:
            /* The purpose of this first call to read_photo_electrons is only
               to retrieve the array and telescope offset numbers (the original offset
               number without ignored telescopes, basically telescope ID minus one), 
               the number of p.e.s and pixels etc. */
            /* Here we expect as well rc = -10 */
            rc = read_photo_electrons(iobuf, H_MAX_PIX, 0, &iarray, &itel_pe,
                  &npe, &pixels, &flags, NULL, NULL, NULL, NULL, NULL);
            if ( rc != -10 )
            {
               get_item_end(iobuf,&item_header);
               return -1;
            }
            /* The itel_pe value may differ from the itel index value that we
               are looking for if the telescope simulation had ignored telescopes.
               This can be fixed but still assumes that base_telescope_number = 1
               was used - as all known simulations do. */
            tel_id = itel_pe + 1; /* Also note: 1 <= tel_id <= 1000 */
            if(is_subarray_selected(tel_id)) {
                itel = tel_id_to_index[tel_id];
            }
            else {
                spdlog::debug("Skip mc photons_electrons for tel_id: {}", tel_id);
                skip_subitem(iobuf);
            }
            if ( itel < 0 || itel >= H_MAX_TEL )
            {
               Warning("Invalid telescope number in MC photons");
               get_item_end(iobuf,&item_header);
               return -1;
            }
            if ( pixels > H_MAX_PIX )
            {
               Warning("Invalid number of pixels in MC photons");
               get_item_end(iobuf,&item_header);
               return -1;
            }
            /* If the current p.e. list buffer is too small or
               non-existent or if it is unnecessarily large, 
               we (re-) allocate a p.e. list buffer for p.e. times
               and, if requested, for amplitudes. */
            if ( npe > mce->mc_pe_list[itel].max_npe || 
                 (npe < mce->mc_pe_list[itel].max_npe/4 && 
                 mce->mc_pe_list[itel].max_npe > 20000) ||
                 mce->mc_pe_list[itel].atimes == NULL ||
                 (mce->mc_pe_list[itel].amplitudes == NULL && (flags&1) != 0) )
            {
               if ( mce->mc_pe_list[itel].atimes != NULL )
                  free(mce->mc_pe_list[itel].atimes);
               if ( (mce->mc_pe_list[itel].atimes = (double *)
                    calloc(npe>0?npe:1,sizeof(double))) == NULL )
               {
                  mce->mc_pe_list[itel].max_npe = 0;
                  get_item_end(iobuf,&item_header);
                  return -4;
               }
               if ( mce->mc_pe_list[itel].amplitudes != NULL )
                  free(mce->mc_pe_list[itel].amplitudes);
               /* If the amplitude bit in flags is set, also check for that part */
               if ( (flags&1) != 0 )
               {
                  if ( (mce->mc_pe_list[itel].amplitudes = (double *)
                       calloc(npe>0?npe:1,sizeof(double))) == NULL )
                  {
                     mce->mc_pe_list[itel].max_npe = 0;
                     get_item_end(iobuf,&item_header);
                     return -4;
                  }
               }
               mce->mc_pe_list[itel].max_npe = max_npe = npe;
            }
            else
               max_npe = mce->mc_pe_list[itel].max_npe;

#ifdef STORE_PHOTO_ELECTRONS
            rc = read_photo_electrons(iobuf, H_MAX_PIX, max_npe, 
                  &iarray, &jtel, &npe, &pixels, &mce->mc_pe_list[itel].flags,
                  mce->mc_pe_list[itel].pe_count, 
                  mce->mc_pe_list[itel].itstart, 
                  mce->mc_pe_list[itel].atimes,
                  mce->mc_pe_list[itel].amplitudes,
                  mce->mc_pe_list[itel].photon_count);
#else
            rc = read_photo_electrons(iobuf, H_MAX_PIX, max_npe, 
                  &iarray, &jtel, &npe, &pixels, &mce->mc_pe_list[itel].flags,
                  mce->mc_pe_list[itel].pe_count, 
                  mce->mc_pe_list[itel].itstart, 
                  mce->mc_pe_list[itel].atimes,
                  mce->mc_pe_list[itel].amplitudes,
                  NULL);
#endif

            mce->mc_pe_list[itel].pixels = pixels;

            if ( rc < 0 )
            {
               mce->mc_pe_list[itel].npe = 0;
               get_item_end(iobuf,&item_header);
               return rc;
            }
            else
            {
               mce->mc_pe_list[itel].npe = npe;
            }

            break;
         default:
            fprintf(stderr,
               "Fix me: unexpected item type %d in read_simtel_mc_phot()\n",type);
            skip_subitem(iobuf);
      }
   }
   
   return end_read_tel_array(iobuf, &item_header);
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
        free(hsdata);
    }
}
