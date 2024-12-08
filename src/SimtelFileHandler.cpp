#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "SimtelFileHandler.hh"
#include "LACT_hessioxxx/include/io_hess.h"
#include "LACT_hessioxxx/include/io_history.h"
#include "LACT_hessioxxx/include/mc_atmprof.h"
#include "LACT_hessioxxx/include/mc_tel.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

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
    open_file(filename);
    _read_history();
    read_metadata();
    read_runheader();
    read_mcrunheader();
    read_atmosphere();
    read_telescope_settings();
    SPDLOG_TRACE("End read simtel file");
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
        SPDLOG_DEBUG("No more blocks");
        no_more_blocks = true;
        return;
    }
    if(read_io_block(iobuf, &item_header) != 0) {
        spdlog::error("Failed to read IO block");
        throw std::runtime_error("Failed to read IO block");
    }
}
void SimtelFileHandler::_read_history() {
    SPDLOG_DEBUG("Read history block");
    read_block();
    // history is the first item in the file
    assert(item_header.type == IO_TYPE_HISTORY);

    // bug in LACT_hessioxxx have been fixed 
    if(read_history(iobuf, &history_container) != 0) {
        spdlog::error("Failed to read history");
        throw std::runtime_error("Failed to read history");
    }
    SPDLOG_DEBUG("End read history block");
}
void SimtelFileHandler::read_metadata() {
    SPDLOG_DEBUG("Read metadata block");
    read_block();
    while(item_header.type == IO_TYPE_METAPARAM) {
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
        read_block();
    }
    SPDLOG_DEBUG("End read metadata block");
}
void SimtelFileHandler::read_telescope_settings() {
    SPDLOG_DEBUG("Begin read telescope settings block");
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
    // Next block have been read already
    SPDLOG_DEBUG("End read telescope settings block");
}
void SimtelFileHandler::read_runheader() {
    SPDLOG_DEBUG("Read runheader block");
    while(item_header.type != IO_TYPE_SIMTEL_RUNHEADER) {
        SPDLOG_DEBUG("Skip block type: {}", item_header.type);
        read_block();
    }
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
    SPDLOG_DEBUG("End read runheader block");

}
void SimtelFileHandler::read_mcrunheader() {
    SPDLOG_DEBUG("Read mcrunheader block");
    while(item_header.type != IO_TYPE_SIMTEL_MCRUNHEADER) {
        SPDLOG_DEBUG("Skip block type: {}", item_header.type);
        read_block();
    }
    if(read_simtel_mcrunheader(iobuf, &hsdata->mc_run_header) < 0) {
        spdlog::error("Failed to read mcrunheader");
        throw std::runtime_error("Failed to read mcrunheader");
    }
    SPDLOG_DEBUG("End read mcrunheader block");
}
void SimtelFileHandler::read_atmosphere() {
    SPDLOG_DEBUG("Read atmosphere block");
    atmprof = get_common_atmprof();
    while(item_header.type != IO_TYPE_MC_ATMPROF) {
        SPDLOG_DEBUG("Skip block type: {}", item_header.type);
        read_block();
    }
    if(read_atmprof(iobuf, atmprof) != 0) {
        spdlog::error("Failed to read atmosphere");
        throw std::runtime_error("Failed to read atmosphere");
    }
    SPDLOG_DEBUG("End read atmosphere block");
}
/**
 * @brief Camera settings is the first item in telescope settings block, we need handle it before others
 * 
 */
void SimtelFileHandler::read_camera_settings() {
    SPDLOG_DEBUG("Read camera settings block");
    // For camera setting, we need read block before call read_simtel_camsettings
    assert(item_header.type == IO_TYPE_SIMTEL_CAMSETTINGS);
    int tel_id = item_header.ident;
    spdlog::debug("Read camera settings for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        SPDLOG_WARN("Skip telescope settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    //memset(&hsdata->camera_set[itel], 0, sizeof(CameraSettings));
    if(read_simtel_camsettings(iobuf, &hsdata->camera_set[itel]) < 0) {
        spdlog::error("Failed to read camera settings");
        throw std::runtime_error("Failed to read camera settings");
    }
    SPDLOG_DEBUG("End read camera settings block");

}
void SimtelFileHandler::read_camera_organisation() {
    SPDLOG_DEBUG("Read camera organisation block");
    while(item_header.type != IO_TYPE_SIMTEL_CAMORGAN) {
        SPDLOG_DEBUG("Skip block type: {}", item_header.type);
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read camera organisation for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        SPDLOG_WARN("Skip camera organisation for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_camorgan(iobuf, &hsdata->camera_org[itel]) < 0) {
        spdlog::error("Failed to read camera organisation");
        throw std::runtime_error("Failed to read camera organisation");
    }
    SPDLOG_DEBUG("End read camera organisation block");
}
void SimtelFileHandler::read_pixel_settings() {
    SPDLOG_DEBUG("Read pixel settings block");
    while(item_header.type != IO_TYPE_SIMTEL_PIXELSET) {
        SPDLOG_DEBUG("Skip block type: {}", item_header.type);
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read pixel settings for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        SPDLOG_WARN("Skip pixel settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_pixelset(iobuf, &hsdata->pixel_set[itel]) < 0) {
        spdlog::error("Failed to read pixel settings");
        throw std::runtime_error("Failed to read pixel settings");
    }   
    SPDLOG_DEBUG("End read pixel settings block");
}
void SimtelFileHandler::read_pixel_disabled() {
    SPDLOG_DEBUG("Read pixel disabled block");
    while(item_header.type != IO_TYPE_SIMTEL_PIXELDISABLE) {
        SPDLOG_DEBUG("Skip block type: {}", item_header.type);
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read pixel disabled for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        SPDLOG_WARN("Skip pixel disabled for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_pixeldis(iobuf, &hsdata->pixel_disabled[itel]) < 0) {
        spdlog::error("Failed to read pixel disabled");
        throw std::runtime_error("Failed to read pixel disabled");
    }
    SPDLOG_DEBUG("End read pixel disabled block");
}

void SimtelFileHandler::read_camera_software_settings() {
    SPDLOG_DEBUG("Read camera software settings block");
    while(item_header.type != IO_TYPE_SIMTEL_CAMSOFTSET) {
        SPDLOG_DEBUG("Skip block type: {}", item_header.type);
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read camera software settings for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        SPDLOG_WARN("Skip camera software settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_camsoftset(iobuf, &hsdata->cam_soft_set[itel]) < 0) {
        spdlog::error("Failed to read camera software settings");
        throw std::runtime_error("Failed to read camera software settings");
    }
    SPDLOG_DEBUG("End read camera software settings block");
}

void SimtelFileHandler::read_pointing_corrections() {
    SPDLOG_DEBUG("Read pointing corrections block");
    while(item_header.type != IO_TYPE_SIMTEL_POINTINGCOR) {
        SPDLOG_DEBUG("Skip block type: {}", item_header.type);
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read pointing corrections for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        SPDLOG_WARN("Skip pointing corrections for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_pointingcor(iobuf, &hsdata->point_cor[itel]) < 0) {
        spdlog::error("Failed to read pointing corrections");
        throw std::runtime_error("Failed to read pointing corrections");
    }
    SPDLOG_DEBUG("End read pointing corrections block");
}

void SimtelFileHandler::read_tracking_settings() {
    SPDLOG_DEBUG("Read tracking settings block");
    while(item_header.type != IO_TYPE_SIMTEL_TRACKSET) {
        SPDLOG_DEBUG("Skip block type: {}", item_header.type);
        read_block();
    }
    int tel_id = item_header.ident;
    spdlog::debug("Read tracking settings for tel_id: {}", tel_id);
    auto it = tel_id_to_index.find(tel_id);
    if(it == tel_id_to_index.end()) {
        SPDLOG_WARN("Skip tracking settings for tel_id: {}", tel_id);
        return;
    }
    int itel = it->second;
    if(read_simtel_trackset(iobuf, &hsdata->tracking_set[itel]) < 0) {
        spdlog::error("Failed to read tracking settings");
        throw std::runtime_error("Failed to read tracking settings");
    }
    SPDLOG_DEBUG("End read tracking settings block");
}
bool SimtelFileHandler::is_subarray_selected(int tel_id) {
    if(allowed_tels.empty()) return true;
    return std::find(allowed_tels.begin(), allowed_tels.end(), tel_id) != allowed_tels.end();
}

void SimtelFileHandler::read_mc_shower() {
    SPDLOG_DEBUG("Read mc shower block");
    // Cause we may need skip some blocks before read mc shower
    while(item_header.type != IO_TYPE_SIMTEL_MC_SHOWER) {
        if(no_more_blocks) return;
        read_block();
        // Use warnning before finishing reading the events block
        //spdlog::warn("Skip block type: {}", item_header.type);
    }
    int run_id = item_header.ident;
    spdlog::debug("Read mc shower for run_id: {}", run_id);
    if(read_simtel_mc_shower(iobuf, &hsdata->mc_shower) != 0) {
        spdlog::error("Failed to read mc shower");
        throw std::runtime_error("Failed to read mc shower");
    }
    read_block();
    SPDLOG_DEBUG("End read mc shower block");
}

void SimtelFileHandler::read_mc_event() {
    assert(item_header.type == IO_TYPE_SIMTEL_MC_EVENT);
    int event_id = item_header.ident;
    spdlog::debug("Read mc event for event_id: {}", event_id);
    if(read_simtel_mc_event(iobuf, &hsdata->mc_event) != 0) {
        spdlog::error("Failed to read mc event");
        throw std::runtime_error("Failed to read mc event");
    }
    read_block();
}
void SimtelFileHandler::read_true_image() {
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
void SimtelFileHandler::load_next_event() {
    read_mc_shower();
    if(no_more_blocks) return;
    read_mc_event();
    read_true_image();
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
