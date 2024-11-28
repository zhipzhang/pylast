#include "SimtelEventSource.hh"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"
#include "nanobind/stl/string.h"
const std::string ihep_url = "root://eos01.ihep.ac.cn/";
SimtelEventSource::SimtelEventSource(const string& filename) : EventSource(filename)
{
    if ( (iobuf = allocate_io_buffer(5000000L)) == NULL )
   {
      spdlog::error("Cannot allocate I/O buffer");
      throw std::runtime_error("Cannot allocate I/O buffer");
   }
    iobuf->output_file = NULL;
    // for 1GB file at least.
    iobuf->max_length  = 1000000000L; 
    open_file(input_filename);
    iobuf->input_file = input_file;
}

void SimtelEventSource::open_file(const string& filename)
{
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
        is_stream = true;
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
        is_stream = true; // Cause for simtel file, it's always be compressed, so it's a stream.
    }
}


const std::string SimtelEventSource::print() const
{
    return spdlog::fmt_lib::format("SimtelEventSource: {}", input_filename);
}

void SimtelEventSource::bind(nb::module_& m)
{
    nb::class_<SimtelEventSource, EventSource>(m, "SimtelEventSource")
        .def(nb::init<const std::string&>(), nb::arg("filename"))
        .def("__repr__", &SimtelEventSource::print);
}
