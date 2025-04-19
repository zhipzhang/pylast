/**
 * @file Convert.cpp
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Convert the RootEventSource to a much simplified version
 * @version 0.1
 * @date 2025-03-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "RootEventSource.hh"
#include "RootWriter.hh"
#include "args.hxx"
#include <iostream>
#include <vector>
#include <string>
#include "Simplied_RootSource.hpp"
#include "TH1F.h"
#include "TH2F.h"
#include "CoordFrames.hh"
#include "Utils.hh"
int main(int argc, const char* argv[])
{
    args::ArgumentParser parser("Convert RootEventSource files to simplified format");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    
    // Define arguments that can be used multiple times
    args::ValueFlagList<std::string> input_files(parser, "input", "Input file (can be specified multiple times)", {'i', "input"});
    args::ValueFlagList<std::string> output_files(parser, "output", "Output file (can be specified multiple times)", {'o', "output"});
    
    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Help&) {
        std::cout << parser;
        return 0;
    } catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    } catch (const args::ValidationError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    
    // Get the input and output files
    const std::vector<std::string>& inputs = input_files.Get();
    const std::vector<std::string>& outputs = output_files.Get();
    
    if (inputs.empty()) {
        std::cerr << "Error: At least one input file must be specified" << std::endl;
        return 1;
    }
    
    if (inputs.size() != outputs.size()) {
        std::cerr << "Error: Number of input files (" << inputs.size() 
                  << ") must match number of output files (" << outputs.size() << ")" << std::endl;
        return 1;
    }
    
    // Process each input-output pair
    for (size_t i = 0; i < inputs.size(); i++) {
        const std::string& input_file = inputs[i];
        const std::string& output_file = outputs[i];
        
        std::cout << "Converting " << input_file << " to " << output_file << std::endl;
        
        try {
            // Open input file
            std::unique_ptr<RootEventSource> source = std::make_unique<RootEventSource>(input_file, -1, std::vector<int>{}, false);
            std::unique_ptr<TFile> output_root = std::unique_ptr<TFile>(TFile::Open(output_file.c_str(), "RECREATE"));
            TTree* teltree = new TTree("tels", "Telescope TTree data");
            TTree* eventtree = new TTree("events", "Event TTree data");
            TelescopeData data;
            EventData event_data;
            initialize_telescope_tree(teltree, data);
            initialize_event_tree(eventtree, event_data);
            auto subarray = source->subarray;
            
            // Rest of your processing code remains the same
            for (const auto& event: *source)
            {
                if(!event.dl2->geometry.at("HillasReconstructor").is_valid )
                {
                    continue;
                }
                event_data.event_id = event.event_id;
                event_data.hillas_n_tels = event.dl2->geometry.at("HillasReconstructor").telescopes.size();
                event_data.hillas_rec_alt = event.dl2->geometry.at("HillasReconstructor").alt;
                event_data.hillas_rec_az = event.dl2->geometry.at("HillasReconstructor").az;
                event_data.hillas_rec_core_x = event.dl2->geometry.at("HillasReconstructor").core_x;
                event_data.hillas_rec_core_y = event.dl2->geometry.at("HillasReconstructor").core_y;
                event_data.hillas_direction_error = event.dl2->geometry.at("HillasReconstructor").direction_error;
                event_data.shower = event.simulation->shower;
                event_data.hillas_hmax = event.dl2->geometry.at("HillasReconstructor").hmax;
                if(event.dl2->geometry.find("DispStereoReconstructor") != event.dl2->geometry.end())
                {
                    event_data.disp_stereo_rec_alt = event.dl2->geometry.at("DispStereoReconstructor").alt;
                    event_data.disp_stereo_rec_az = event.dl2->geometry.at("DispStereoReconstructor").az;
                    event_data.disp_direction_error = event.dl2->geometry.at("DispStereoReconstructor").direction_error;
                }
                if(!event.dl2->energy.empty())
                {
                    event_data.rec_energy = event.dl2->energy.at("MLEnergyReconstructor").estimate_energy;
                }
                else
                {
                    event_data.rec_energy = 0;
                }
                eventtree->Fill();
                double average_intensity_sum = 0;
                for (const auto& [tel_id, dl1]: event.dl1->tels)
                {
                    average_intensity_sum += dl1->image_parameters.hillas.intensity;
                }
                double average_intensity = average_intensity_sum / event.dl1->tels.size();
                for(const auto& [tel_id, rec_impact]: event.dl2->tels)
                {
                    auto tel_coord = subarray->tel_positions.at(tel_id);
                    std::array<double, 3> true_core = {event.simulation->shower.core_x, event.simulation->shower.core_y, 0};
                    auto true_direction = SkyDirection(AltAzFrame(), event.simulation->shower.az, event.simulation->shower.alt)->transform_to_cartesian();
                    std::array<double, 3> line_direction = {true_direction.direction[0], true_direction.direction[1], true_direction.direction[2]};
                    auto impact_parameter = Utils::point_line_distance(tel_coord, true_core, line_direction);
                    data.event_id = event.event_id;
                    data.tel_id = tel_id;
                    data.rec_impact_parameter = rec_impact.impact_parameters.at("HillasReconstructor").distance;
                    data.true_impact_parameter = impact_parameter;
                    data.params = event.dl1->tels.at(tel_id)->image_parameters;
                    data.true_alt = event.simulation->shower.alt;
                    data.true_az = event.simulation->shower.az;
                    data.true_energy = event.simulation->shower.energy;
                    data.rec_alt = event.dl2->geometry.at("HillasReconstructor").alt;
                    data.rec_az = event.dl2->geometry.at("HillasReconstructor").az;
                    data.average_intensity = average_intensity;
                    if(!event.dl2->energy.empty())
                    {
                        data.rec_energy = event.dl2->energy.at("MLEnergyReconstructor").estimate_energy;
                        data.tel_rec_energy = event.dl2->tels.at(tel_id).estimate_energy;
                    }
                    else
                    {
                        data.rec_energy = 0;
                        data.tel_rec_energy = 0;
                    }
                    data.n_tel = event.dl2->geometry.at("HillasReconstructor").telescopes.size();
                    teltree->Fill();
                }
            }
            
            if(source->statistics.has_value())
            {
                int ihist = 0;
                auto statistics = source->statistics.value();
                for(const auto& [name, hist]: statistics.histograms)
                {
                    if(hist->get_dimension() == 1)
                    {
                        auto h1d = dynamic_cast<Histogram1D<float>*>(hist.get());
                        auto new_hist = new TH1F(("h" + std::to_string(ihist)).c_str(), name.c_str(), h1d->bins(), 
                                             h1d->get_low_edge(), h1d->get_high_edge());
                        for(int i = 0; i < h1d->bins(); i++)
                        {
                            new_hist->SetBinContent(i+1, h1d->get_bin_content(i));
                        }
                        new_hist->Write();
                        ihist++;
                    }
                    else if(hist->get_dimension() == 2)
                    {
                        auto h2d = dynamic_cast<Histogram2D<float>*>(hist.get());
                        auto new_hist = new TH2F(("h" + std::to_string(ihist)).c_str(), name.c_str(), 
                                             h2d->x_bins(), h2d->get_x_low_edge(), h2d->get_x_high_edge(),
                                             h2d->y_bins(), h2d->get_y_low_edge(), h2d->get_y_high_edge());
                        for(int i = 0; i < h2d->x_bins(); i++)
                        {
                            for(int j = 0; j < h2d->y_bins(); j++)
                            {
                                new_hist->SetBinContent(i+1, j+1, h2d->operator()(i, j));
                            }
                        }
                        new_hist->Write();
                        ihist++;
                    }
                }
            }
            output_root->Write();
        }
        catch (const std::exception& e) {
            std::cerr << "Error processing " << input_file << ": " << e.what() << std::endl;
            continue;  // Continue with next file instead of stopping
        }
    }
    
    return 0;
}