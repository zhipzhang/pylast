/**
 * @file DataWriter.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  Data writer for different file formats
 * @version 0.1
 * @date 2025-02-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #pragma once
 #include "Configurable.hh"
 #include "ArrayEvent.hh"
 #include "EventSource.hh"
#include "spdlog/pattern_formatter-inl.h"


 class FileWriter
 {
    public:
        FileWriter(EventSource& source, const std::string& filename):
            source(source),
            filename(filename)
        {};
        virtual ~FileWriter() = default;
        virtual void open(bool overwrite = false) = 0;
        virtual void close() = 0;
        virtual void write_atmosphere_model() = 0;
        virtual void write_subarray() = 0;
        //virtual void write_simulation_config() = 0;
    protected:
        EventSource& source;
        std::string filename;
 };
 
 class DataWriter: public Configurable
 {
    public:
        DECLARE_CONFIGURABLE_DOUBLE_DEFINITIONS (EventSource&, source, const std::string&, filename, DataWriter);
        virtual ~DataWriter() 
        {
            if(file_writer)
            {
                file_writer->close();
            }
        }
        void configure(const json& config) override;
        static json get_default_config();
        json default_config() const override {return get_default_config();}
        //virtual void operator()(const ArrayEvent& event) = 0;
    protected:
        EventSource& source;
        std::string filename;
    private:
        std::unique_ptr<FileWriter> file_writer;
 };