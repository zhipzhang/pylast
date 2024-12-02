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
#include "LACT_hessioxxx/include/io_hess.h"
#include "LACT_hessioxxx/include/fileopen.h"
#include "LACT_hessioxxx/include/io_hess.h"
#include "LACT_hessioxxx/include/io_basic.h"
#include "LACT_hessioxxx/include/io_history.h"
#include "LACT_hessioxxx/include/mc_atmprof.h"
#include "LACT_hessioxxx/include/initial.h"
#include <vector>
#include <unordered_map>
class SimtelFileHandler {
    friend class SimtelEventSource;
public:
    SimtelFileHandler(const std::string& filename, std::vector<int> subarray = {});
    SimtelFileHandler() = default;
    ~SimtelFileHandler();
private:
    std::string filename = "none";
    FILE* input_file = nullptr;
    IO_BUFFER* iobuf = nullptr;
    IO_ITEM_HEADER item_header;
    AllHessData* hsdata = nullptr;
    std::vector<int> allowed_tels;
    std::unordered_map<int, int> tel_id_to_index;
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
    AtmProf* atmprof;
    
};
