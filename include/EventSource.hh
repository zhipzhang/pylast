/**
 * @file BaseRawSource.hh
 * @author Zach Peng
 * @brief Base class to read raw event files
 * @version 0.1
 * @date 2024-11-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#pragma once

#include <string>
#include <format>
#include "nanobind/nanobind.h"

namespace nb = nanobind;
using std::string;
class EventSource
{
public:
    EventSource() = default;
    EventSource(const string& filename) : input_filename(filename) {}
    virtual ~EventSource() = default;
    virtual void open_file(const string& filename) = 0;
    string input_filename;

    /** @brief Whether the file is a stream(e.g. eos file system) */
    bool is_stream = false;

    static void bind(nb::module_& m) {
        nb::class_<EventSource>(m, "EventSource")
            .def_ro("input_filename", &EventSource::input_filename)
            .def_ro("is_stream", &EventSource::is_stream);
    }
};

