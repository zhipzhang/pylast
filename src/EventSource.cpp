#include "EventSource.hh"
#include <algorithm>

bool EventSource::is_subarray_selected(int tel_id) const
{
    if(subarray.empty()) return true;
    return std::find(subarray.begin(), subarray.end(), tel_id) != subarray.end();
}