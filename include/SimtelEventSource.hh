/**
 * @file SimtelEventSource.hh
 * @author Zach Peng
 * @brief Class to read simtel files
 * @version 0.1
 * @date 2024-11-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include "LACT_hessioxxx/include/io_hess.h"
#include "LACT_hessioxxx/include/fileopen.h"
#include "LACT_hessioxxx/include/io_hess.h"
#include "LACT_hessioxxx/include/io_basic.h"
#include "EventSource.hh"
#include <unordered_map>
class SimtelEventSource: public EventSource 
{
public:
    SimtelEventSource() = default;
    SimtelEventSource(const string& filename, int64_t max_events = -1 , std::vector<int> subarray = {});
    ~SimtelEventSource() = default;
    static void bind(nb::module_& m);
private:
    void open_file(const string& filename) override;
    FILE* input_file = nullptr;
    IO_BUFFER* iobuf = nullptr;
    IO_ITEM_HEADER item_header;
    const std::string print() const;
    AllHessData* hsdata = nullptr;
    void read_block();
    virtual void init_simulation_config() override;
    //virtual void init_subarray() override;
    void set_simulation_config();
    void read_runheader();
    void read_mcrunheader();
    std::unordered_map<int, int> tel_id_to_index;
};

