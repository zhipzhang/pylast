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
#include <vector>
#include <unordered_map>
using History_Entry = std::pair<time_t, std::string>;
using History_List = std::vector<History_Entry>;
class SimtelFileHandler {
    friend class SimtelEventSource;
public:
    SimtelFileHandler(const std::string& filename, std::vector<int> subarray = {});
    SimtelFileHandler() = default;
    ~SimtelFileHandler();
    bool no_more_blocks = false;
    bool have_true_image = false;
private:
    std::string filename = "none";
    FILE* input_file = nullptr;
    IO_BUFFER* iobuf = nullptr;
    IO_ITEM_HEADER item_header;
    AllHessData* hsdata = nullptr;
    std::vector<int> allowed_tels;
    std::unordered_map<int, int> tel_id_to_index;
    std::unordered_map<std::string, std::string> global_metadata;
    std::unordered_map<int, std::unordered_map<std::string, std::string>> tel_metadata;
    bool is_subarray_selected(int tel_id);
    void open_file(const std::string& filename);
    void read_block();
    void read_runheader();
    void read_mcrunheader();
    void read_atmosphere();
    void read_camera_settings();
    void read_camera_organisation();
    void read_pixel_settings();
    void read_pixel_disabled();
    void read_camera_software_settings();
    void read_pointing_corrections();
    void read_tracking_settings();
    void read_telescope_settings();
    void read_mc_shower();
    void read_mc_event();
    void read_true_image();
    void read_metadata();
    void _read_history();
    void load_next_event();
    /**
     * @brief In order to use an native skip function of telescope, we borrow the read_simtel_mc_phot function from io_hess.c and change a little bit.
     * 
     * @param iobuf 
     * @param mce 
     * @return int 
     */
    int _read_simtel_mc_phot(IO_BUFFER* iobuf, MCEvent* mce);
    AtmProf* atmprof;
    HistoryContainer history_container;
    MetaParamList metadata_list;
};
