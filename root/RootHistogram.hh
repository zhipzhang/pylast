#pragma once

#include "Statistics.hh"
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TFile.h>
#include <TKey.h>
#include <TDirectory.h>
#include <memory>
#include <string>

class RootHistogram {
public:
    RootHistogram() = default;
    ~RootHistogram() = default;

    // Write histograms from Statistics to ROOT file
    static void write_statistics(const Statistics& stats, const std::string& filename) {
        auto file = TFile::Open(filename.c_str(), "RECREATE");
        
        int ihist = -1;
        for (const auto& [name, hist] : stats.histograms) {
            ihist++;
            // Try to convert to different histogram types
            if(hist->get_dimension() == 1)
            {
                if (auto hist1d = std::dynamic_pointer_cast<Histogram1D<float>>(hist)) {
                    write_histogram1d(name, *hist1d, ihist);
                }
            }
            else if(hist->get_dimension() == 2)
            {
                if (auto hist2d = std::dynamic_pointer_cast<Histogram2D<float>>(hist)) {
                    write_histogram2d(name, *hist2d, ihist);
                }
            }
            else if(hist->get_dimension() == 0)
            {
                if (auto profile = std::dynamic_pointer_cast<Profile1D<float>>(hist)) {
                    write_profile(name, *profile, ihist);
                }
            }
        }
        
        file->Write();
        file->Close();
    }

    // Load histograms from ROOT file into Statistics
    static void load_statistics(Statistics& stats, const std::string& filename) {
        TFile file(filename.c_str(), "READ");
        
        // Check if file is open
        if (!file.IsOpen()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        TList* keys = file.GetListOfKeys();
        TIter next(keys);
        TKey* key;
        
        while ((key = (TKey*)next())) {
            std::string name = key->GetName();
            TObject* obj = file.Get(name.c_str());
            
            if (auto th1d = dynamic_cast<TH1D*>(obj)) {
                load_histogram1d(stats, name, th1d);
            }
            else if (auto th2d = dynamic_cast<TH2D*>(obj)) {
                load_histogram2d(stats, name, th2d);
            }
            else if (auto tprofile = dynamic_cast<TProfile*>(obj)) {
                load_profile(stats, name, tprofile);
            }
        }
        
        file.Close();
    }

private:
    // Helper methods for writing histograms
    static void write_histogram1d(const std::string& name, const Histogram1D<float>& hist, int ihist) {
        int bins = hist.bins();
        float min = hist.get_low_edge();
        float max = hist.get_high_edge();
        
        TH1D* th1d = new TH1D( ("h" + std::to_string(ihist)).c_str(), name.c_str(), bins, min, max);
        
        // Fill the ROOT histogram
        for (int i = 0; i < bins; ++i) {
            th1d->SetBinContent(i + 1, hist.at(i));
        }
        
        th1d->Write();
        delete th1d;
    }

    static void write_histogram2d(const std::string& name, const Histogram2D<float>& hist, int ihist) {
        int x_bins = hist.x_bins();
        int y_bins = hist.y_bins();
        float x_min = hist.get_x_low_edge();
        float x_max = hist.get_x_high_edge();
        float y_min = hist.get_y_low_edge();
        float y_max = hist.get_y_high_edge();
        
        TH2D* th2d = new TH2D( ("h" + std::to_string(ihist)).c_str(), name.c_str(), 
                             x_bins, x_min, x_max,
                             y_bins, y_min, y_max);
        
        // Fill the ROOT histogram
        for (int i = 0; i < x_bins; ++i) {
            for (int j = 0; j < y_bins; ++j) {
                th2d->SetBinContent(i + 1, j + 1, hist.at(i, j));
            }
        }
        
        th2d->Write();
        delete th2d;
    }

    static void write_profile(const std::string& name, const Profile1D<float>& profile, int ihist) {
        int bins = profile.bins();
        float min = profile.get_low_edge();
        float max = profile.get_high_edge();
        
        TProfile* tprofile = new TProfile(("h" + std::to_string(ihist)).c_str(), name.c_str(), bins, min, max);
        
        // Fill the ROOT profile with known values
        for (int i = 0; i < bins; ++i) {
            if(profile.mean(i) != 0)
            {
                // Set bin content and error directly
                tprofile->SetBinEntries(i + 1, 1);
                tprofile->SetBinContent(i + 1, profile.mean(i));
                tprofile->SetBinError(i + 1, profile.error(i));
                // Set bin entries to a non-zero value to make the profile valid
            }
        }
        
        tprofile->Write();
        delete tprofile;
    }

    // Helper methods for loading histograms
    static void load_histogram1d(Statistics& stats, const std::string& name, TH1D* th1d) {
        int bins = th1d->GetNbinsX();
        float min = th1d->GetXaxis()->GetBinLowEdge(1);
        float max = th1d->GetXaxis()->GetBinUpEdge(bins);
        
        auto hist = make_regular_histogram<float>(min, max, bins);
        
        // Fill the custom histogram
        for (int i = 0; i < bins; ++i) {
            hist.fill(th1d->GetBinCenter(i + 1), th1d->GetBinContent(i + 1));
        }
        
        stats.add_histogram(name, std::move(hist));
    }

    static void load_histogram2d(Statistics& stats, const std::string& name, TH2D* th2d) {
        int x_bins = th2d->GetNbinsX();
        int y_bins = th2d->GetNbinsY();
        float x_min = th2d->GetXaxis()->GetBinLowEdge(1);
        float x_max = th2d->GetXaxis()->GetBinUpEdge(x_bins);
        float y_min = th2d->GetYaxis()->GetBinLowEdge(1);
        float y_max = th2d->GetYaxis()->GetBinUpEdge(y_bins);
        
        auto hist = make_regular_histogram_2d<float>(x_min, x_max, x_bins,
                                                   y_min, y_max, y_bins);
        
        // Fill the custom histogram
        for (int i = 0; i < x_bins; ++i) {
            for (int j = 0; j < y_bins; ++j) {
                hist.fill(th2d->GetXaxis()->GetBinCenter(i + 1),
                         th2d->GetYaxis()->GetBinCenter(j + 1),
                         th2d->GetBinContent(i + 1, j + 1));
            }
        }
        
        stats.add_histogram(name, std::move(hist));
    }

    static void load_profile(Statistics& stats, const std::string& name, TProfile* tprofile) {
        int bins = tprofile->GetNbinsX();
        float min = tprofile->GetXaxis()->GetBinLowEdge(1);
        float max = tprofile->GetXaxis()->GetBinUpEdge(bins);
        
        auto profile = make_regular_profile<float>(min, max, bins);
        
        // Fill the custom profile
        for (int i = 0; i < bins; ++i) {
            profile.fill(tprofile->GetBinCenter(i + 1),
                        tprofile->GetBinContent(i + 1),
                        tprofile->GetBinEntries(i + 1));
        }
        
        stats.add_histogram(name, std::move(profile));
    }
};
