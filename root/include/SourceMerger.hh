#pragma once


#include "TFileMerger.h"
#include <memory>
#include "TFile.h"


class SourceMerger{
    public:
        SourceMerger(const std::string& output_file);
        ~SourceMerger() = default;
        void operator()(std::vector<std::string> input_files, bool is_root = true);
        void merge();
        void handle_root_subarray(std::string output_file, std::string input_file);
        static void copy_directory(TDirectory* source, TDirectory* target);
        void build_index_for_direcrory(const std::string& directory_name);
    private:
        std::unique_ptr<TFileMerger> merger;
        std::string output_file;
        std::unique_ptr<TFile> output_root;
};