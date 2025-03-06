#include "DataWriter.hh"
#include "DataWriterFactory.hh"
json DataWriter::get_default_config()
{
    std::string default_config = R"(
    {
        "output_type": "root",
        "eos_url": "root://eos01.ihep.ac.cn/",
        "overwrite": false,
        "write_r0_waveforms": false,
        "write_r1_waveforms": false,
        "write_dl0_images": false,
        "write_dl1_images": true,
        "write_dl1_parameters": true,
        "write_dl2": true,
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
    if(config.at("write_atmosphere_model") )
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
}