#include "DataFrame.hh"
#include "RootEventSource.hh"
#include <chrono>
#include <iostream>

int main(int argc, char **argv) {
    auto filename = argv[1];
    std::cout << "Input file: " << filename << std::endl;
    auto event_source = new RootEventSource(filename,-1,{});
    std::cout << "max event: " << event_source->max_events << std::endl;
    df::DataFrameMaker df_maker(*event_source);
    auto time = std::chrono::high_resolution_clock::now();    
    auto table = df_maker();
    auto diff = (std::chrono::high_resolution_clock::now() - time);
    std::cout
        << "Time taken to create Table: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()
        << " milliseconds" << std::endl;
    //df::MultiThreadedDataFrameMaker mdf_maker(*event_source);

    // time = std::chrono::high_resolution_clock::now();
    // mdf_maker.build_table();
    // diff = (std::chrono::high_resolution_clock::now() - time);

    // std::cout << "Time taken to create MultiThreaded Table: "
    //           << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()
    //           << " milliseconds" << std::endl;

    // if (table) {
    //     std::cout << "Table created successfully!" << std::endl;
    // } else {
    //     std::cout << "Failed to create Table." << std::endl;
    // }
    return 0;
}