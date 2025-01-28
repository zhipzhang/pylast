#include "EventMonitor.hh"
#include "BaseTelContainer.hh"
void EventMonitor::add_telmonitor(int tel_id, int n_channels, int n_pixels, double* pedestal_per_sample, double* dc_to_pe, int max_pixels)
{
    BaseTelContainer::add_tel(tel_id, n_channels, n_pixels, pedestal_per_sample, dc_to_pe, max_pixels);
}