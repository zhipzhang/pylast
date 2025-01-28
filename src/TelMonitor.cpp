#include "TelMonitor.hh"
#include "Eigen/src/Core/Map.h"

TelMonitor::TelMonitor(int n_channels, int n_pixels, double* pedestal_per_sample, double* dc_to_pe, int max_pixels)
{
    if(pedestal_per_sample == nullptr || dc_to_pe == nullptr) {
        throw std::runtime_error("Pedestal or DC to PE is not available");
    }
    this->n_channels = n_channels;
    this->n_pixels = n_pixels;
    for(int ichannel = 0; ichannel < n_channels; ichannel++) {
        this->pedestal_per_sample[ichannel].resize(n_pixels);
        this->dc_to_pe[ichannel].resize(n_pixels);
        for(int ipixel = 0; ipixel < n_pixels; ipixel++) {
            this->pedestal_per_sample[ichannel](ipixel) = pedestal_per_sample[ichannel * max_pixels + ipixel];
            this->dc_to_pe[ichannel](ipixel) = dc_to_pe[ichannel * max_pixels + ipixel];
        }
    }
}