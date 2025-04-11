#include "DataWriter.hh"
#include "DataWriterFactory.hh"
json DataWriter::get_default_config()
{
    std::string default_config = R"(
    {
        "output_type": "root",
        "eos_url": "root://eos01.ihep.ac.cn/",
        "overwrite": true,
        "write_simulation_shower": true,
        "write_simulated_camera": false,
        "write_r0": true,
        "write_r1": true,
        "write_dl0": true,
        "write_dl1": true,
        "write_dl1_image": false,
        "write_dl2": true,
        "write_monitor": true,
        "write_pointing": true,
        "write_simulation_config": true,
        "write_atmosphere_model": true,
        "write_subarray": true,
        "write_metaparam": true
    }
    )";
    json base_config = Configurable::from_string(default_config);
    return base_config;
}

void DataWriter::configure(const json& config)
{
    std::string output_type = config.at("output_type");
    if(filename.find("/eos") != std::string::npos)
    {
        filename = std::string(config.at("eos_url")) + filename;
    }
    file_writer = DataWriterFactory::instance().create(output_type, source, filename);
    file_writer->open(config.at("overwrite"));
    if(config.at("write_atmosphere_model"))
    {
        file_writer->write_atmosphere_model();
    }
    if(config.at("write_subarray"))
    {
        file_writer->write_subarray();
    }
    if(config.at("write_simulation_config"))
    {
        //file_writer->write_simulation_config();
    }
    
    // Store configuration for selective component writing
    write_simulation_shower_enabled = config.value("write_simulation_shower", true);
    write_simulated_camera_enabled = config.value("write_simulated_camera", false);
    write_r0_enabled = config.value("write_r0", false);
    write_r1_enabled = config.value("write_r1", false);
    write_dl0_enabled = config.value("write_dl0", false);
    write_dl1_enabled = config.value("write_dl1", true);
    write_dl1_image_enabled = config.value("write_dl1_image", false);
    write_dl2_enabled = config.value("write_dl2", true);
    write_monitor_enabled = config.value("write_monitor", false);
    write_pointing_enabled = config.value("write_pointing", false);
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
        file_writer->write_simulated_camera(event);
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
void DataWriter::write_statistics(const Statistics& statistics)
{
    if(file_writer)
    {
        file_writer->write_statistics(statistics);
    }
}