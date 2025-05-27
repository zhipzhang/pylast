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
 #include "Statistics.hh"


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
        virtual void write_simulation_config() = 0;
        virtual void write_subarray() = 0;
        virtual void unique_write_method(const ArrayEvent& event) = 0;
        
        // Methods for writing specific parts of an ArrayEvent
        virtual void write_simulation_shower(const ArrayEvent& event) = 0;
        virtual void write_simulated_camera(const ArrayEvent& event, bool write_image = false) = 0;
        virtual void write_r0(const ArrayEvent& event) = 0;
        virtual void write_r1(const ArrayEvent& event) = 0;
        virtual void write_dl0(const ArrayEvent& event) = 0;
        virtual void write_dl1(const ArrayEvent& event, bool write_image = false) = 0;
        virtual void write_dl2(const ArrayEvent& event) = 0;
        virtual void write_monitor(const ArrayEvent& event) = 0;
        virtual void write_pointing(const ArrayEvent& event) = 0;
        
        // Write all parts of the event (or those enabled in configuration)
        virtual void write_event(const ArrayEvent& event) = 0;
        virtual void write_statistics(const Statistics& statistics) = 0;
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
            close();
        }
        void close()
        {
            if(file_writer)
            {
                file_writer->close();
                file_writer = nullptr;
            }
        }
        void configure(const json& config) override;
        static json get_default_config();
        json default_config() const override {return get_default_config();}
        
        // Methods for writing specific parts of an ArrayEvent
        void write_simulation_shower(const ArrayEvent& event);
        void write_simulated_camera(const ArrayEvent& event);
        void write_r0(const ArrayEvent& event);
        void write_r1(const ArrayEvent& event);
        void write_dl0(const ArrayEvent& event);
        void write_dl1(const ArrayEvent& event);
        void write_dl2(const ArrayEvent& event);
        void write_monitor(const ArrayEvent& event);
        void write_pointing(const ArrayEvent& event);
        void write_statistics(const Statistics& statistics);
        
        // Write all parts of the event (or those enabled in configuration)
        //void write_event(const ArrayEvent& event);
        void operator()(const ArrayEvent& event);
    protected:
        EventSource& source;
        std::string filename;
        
        // Flags to control which components to write
        bool write_simulation_shower_enabled = true;
        bool write_simulated_camera_enabled = true;
        bool write_simulated_camera_image_enabled = false;
        bool write_r0_enabled = false;
        bool write_r1_enabled = false;
        bool write_dl0_enabled = false;
        bool write_dl1_enabled = true;
        bool write_dl1_image_enabled = false;
        bool write_dl2_enabled = true;
        bool write_monitor_enabled = false;
        bool write_pointing_enabled = false;
        
    private:
        std::unique_ptr<FileWriter> file_writer;
 };