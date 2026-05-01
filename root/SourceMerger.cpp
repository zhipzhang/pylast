#include "RtypesCore.h"
#include "SourceMerger.hh"
#include "TTree.h"
#include "TKey.h"
#include "TROOT.h"
#include "TFile.h"
#include "spdlog/spdlog.h"
SourceMerger::SourceMerger(const std::string& output_file)
    : output_file(output_file)
{
}


void SourceMerger::operator()(std::vector<std::string> input_files, bool is_root)
{
    if(input_files.size() < 2)
    {
        spdlog::warn("less than 2 input files, don't need to merge");
        throw std::runtime_error("less than 2 input files, don't need to merge");
    }
    if(is_root)
    {
        merger = std::make_unique<TFileMerger>(kFALSE);
        merger->OutputFile(output_file.c_str());
        for(const auto& input_file : input_files)
        {
            merger->AddFile(input_file.c_str());
        }
        if(! merger->Merge())
        {
            throw std::runtime_error("failed to merge files");
        }
        output_root = std::make_unique<TFile>(output_file.c_str(), "UPDATE");
        build_index_for_direcrory("events/c0");
        build_index_for_direcrory("events/c1");
        build_index_for_direcrory("events/r0");
        build_index_for_direcrory("events/r1");
        build_index_for_direcrory("events/dl0");
        build_index_for_direcrory("events/dl1");
        build_index_for_direcrory("events/dl2");
        build_index_for_direcrory("events/monitor");
        build_index_for_direcrory("events/simulation");
        build_index_for_direcrory("events/pointing");
        output_root->Write();
        output_root->Close();
    }
    else
    {
        // Using the SimtelEventSource and Writer to merge;  just skipped right now.
        spdlog::warn("merge for simtel files is not implemented yet");
        throw std::runtime_error("merge for simtel files is not implemented yet");
    }
}

/**
 * @brief 辅助函数：将 source 目录下的所有对象（含 TTree 和子目录）深拷贝到 target 目录
 */
 void SourceMerger::copy_directory(TDirectory *source, TDirectory *target) {
    target->cd();
    TIter nextkey(source->GetListOfKeys());
    TKey *key;
    while ((key = (TKey*)nextkey())) {
        const char *classname = key->GetClassName();
        TClass *cl = gROOT->GetClass(classname);
        if (!cl) continue;
        
        if (cl->InheritsFrom(TDirectory::Class())) {
            source->cd(key->GetName());
            TDirectory *subdir = gDirectory;
            target->cd();
            TDirectory *newdir = target->mkdir(key->GetName());
            SourceMerger::copy_directory(subdir, newdir);
        } else if (cl->InheritsFrom(TTree::Class())) {
            TTree *T = (TTree*)source->Get(key->GetName());
            target->cd();
            TTree *newT = T->CloneTree(-1, "fast");
            newT->Write();
        } else {
            source->cd();
            TObject *obj = key->ReadObj();
            target->cd();
            obj->Write();
            delete obj;
        }
    }
}
void SourceMerger::handle_root_subarray(std::string output_file, std::string input_file)
{
    std::unique_ptr<TFile> out_root = std::make_unique<TFile>(output_file.c_str(), "UPDATE");
    if(!out_root || out_root->IsZombie())
    {
        throw std::runtime_error("failed to open output file: " + output_file);
    }
    out_root->Delete("subarray;*");
    std::unique_ptr<TFile> in_root = std::make_unique<TFile>(input_file.c_str(), "READ");
    if(!in_root || in_root->IsZombie())
    {
        throw std::runtime_error("failed to open input file: " + input_file);
    }
    TDirectory *subarray_dir = in_root->GetDirectory("subarray");
    if(subarray_dir)
    {
        out_root->cd();
        TDirectory *new_subarray_dir = out_root->mkdir("subarray");
        SourceMerger::copy_directory(subarray_dir, new_subarray_dir);
        out_root->Write();
        spdlog::info("subarray directory restored successfully");
    }
    else
    {
        spdlog::warn("no subarray directory found in input file");
    }
    in_root->Close();
    out_root->Close();
}

void SourceMerger::build_index_for_direcrory(const std::string& directory_name)
{
    if(!output_root || output_root->IsZombie())
    {
        throw std::runtime_error("failed to open output file: " + output_file);
    }
    TDirectory *directory = output_root->GetDirectory(directory_name.c_str());
    if(!directory)
    {
        return;
    }
    directory->cd();
    TTree *tree = (TTree*)directory->Get("tels");
    if(!tree)
    {
        throw std::runtime_error("tree not found in directory: " + directory_name);
    }
    tree->BuildIndex("event_id", "tel_id");
}