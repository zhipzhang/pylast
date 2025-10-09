#include "DataWriter.hh"
#include "DataWriterFactory.hh"

void DataWriter::registerParams()
{
    // Register all configuration parameters using the new system
    registerParam<std::string>("output_type", "root", output_type);
    registerParam<std::string>("eos_url", "root://eos01.ihep.ac.cn/", eos_url);
    registerParam<bool>("overwrite", true, overwrite);
    registerParam<bool>("write_simulation_shower", true, write_simulation_shower_enabled);
    registerParam<bool>("write_simulated_camera", true, write_simulated_camera_enabled);
    registerParam<bool>("write_simulated_camera_image", false, write_simulated_camera_image_enabled);
    registerParam<bool>("write_r0", false, write_r0_enabled);
    registerParam<bool>("write_r1", false, write_r1_enabled);
    registerParam<bool>("write_dl0", false, write_dl0_enabled);
    registerParam<bool>("write_dl1", true, write_dl1_enabled);
    registerParam<bool>("write_dl1_image", false, write_dl1_image_enabled);
    registerParam<bool>("write_dl2", true, write_dl2_enabled);
    registerParam<bool>("write_monitor", false, write_monitor_enabled);
    registerParam<bool>("write_pointing", false, write_pointing_enabled);
    registerParam<bool>("write_atmosphere_model", false, write_atmosphere_model_enabled);
    registerParam<bool>("write_subarray", true, write_subarray_enabled);
    registerParam<bool>("write_simulation_config", true, write_simulation_config_enabled);
}

void DataWriter::setUp()
{
    if(filename.find("/eos") != std::string::npos)
    {
        filename = eos_url + filename;
    }
    file_writer = DataWriterFactory::instance().create(output_type, source, filename);
    file_writer->open(overwrite);
    if(write_atmosphere_model_enabled)
    {
        file_writer->write_atmosphere_model();
    }
    if(write_subarray_enabled)
    {
        file_writer->write_subarray();
    }
    if(write_simulation_config_enabled)
    {
        file_writer->write_simulation_config();
    }
}

void DataWriter::operator()(const ArrayEvent& event)
{
    if(!file_writer)
        return;
    
    // Only write enabled components
    file_writer->unique_write_method(event);
    if(write_simulation_shower_enabled && event.simulation.has_value())
    {
        file_writer->write_simulation_shower(event);
    }
    if(write_simulated_camera_enabled && event.simulation->tels.size() > 0)
    {
        file_writer->write_simulated_camera(event, write_simulated_camera_image_enabled);
    }
    if(write_r0_enabled && event.r0.has_value())
    {
        file_writer->write_r0(event);
    }
    
    if(write_r1_enabled && event.r1.has_value())
    {
        file_writer->write_r1(event);
    }
    
    if(write_dl0_enabled && event.dl0.has_value())
    {
        file_writer->write_dl0(event);
    }
    
    if(write_dl1_enabled && event.dl1.has_value())
    {
        file_writer->write_dl1(event, write_dl1_image_enabled);
    }
    
    if(write_dl2_enabled && event.dl2.has_value())
    {
        file_writer->write_dl2(event);
    }
    
    if(write_monitor_enabled && event.monitor.has_value())
    {
        file_writer->write_monitor(event);
    }
    
    if(write_pointing_enabled && event.pointing.has_value())
    {
        file_writer->write_pointing(event);
    }
}

void DataWriter::write_simulation_shower(const ArrayEvent& event)
{
    if(file_writer)
    {
        file_writer->write_simulation_shower(event);
    }
}

void DataWriter::write_simulated_camera(const ArrayEvent& event)
{
    if(file_writer)
    {
        file_writer->write_simulated_camera(event);
    }
}
void DataWriter::write_r0(const ArrayEvent& event)
{
    if(file_writer)
    {
        file_writer->write_r0(event);
    }
}

void DataWriter::write_r1(const ArrayEvent& event)
{
    if(file_writer)
    {
        file_writer->write_r1(event);
    }
}

void DataWriter::write_dl0(const ArrayEvent& event)
{
    if(file_writer)
    {
        file_writer->write_dl0(event);
    }
}

void DataWriter::write_dl1(const ArrayEvent& event)
{
    if(file_writer)
    {
        file_writer->write_dl1(event);
    }
}

void DataWriter::write_dl2(const ArrayEvent& event)
{
    if(file_writer)
    {
        file_writer->write_dl2(event);
    }
}

void DataWriter::write_monitor(const ArrayEvent& event)
{
    if(file_writer)
    {
        file_writer->write_monitor(event);
    }
}

void DataWriter::write_pointing(const ArrayEvent& event)
{
    if(file_writer)
    {
        file_writer->write_pointing(event);
    }
}
void DataWriter::write_statistics(const Statistics& statistics, bool last)
{
    if(file_writer)
    {
        file_writer->write_statistics(statistics, last);
    }
}