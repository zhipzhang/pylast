#pragma once

#include "LACT_hessioxxx/include/io_hess.h"
#include "LACT_hessioxxx/include/fileopen.h"
#include "LACT_hessioxxx/include/io_hess.h"
#include "LACT_hessioxxx/include/io_basic.h"
#include "EventSource.hh"

class SimtelEventSource: public EventSource 
{
public:
    SimtelEventSource() = default;
    SimtelEventSource(const string& filename);
    ~SimtelEventSource() = default;
    static void bind(nb::module_& m);
private:
    void open_file(const string& filename) override;
    FILE* input_file = nullptr;
    IO_BUFFER* iobuf = nullptr;
    IO_ITEM_HEADER item_header;
    const std::string print() const;
};

