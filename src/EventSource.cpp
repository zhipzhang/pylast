#include "EventSource.hh"
#include <algorithm>

bool EventSource::is_subarray_selected(int tel_id) const
{
    if(allowed_tels.empty()) return true;
    return std::find(allowed_tels.begin(), allowed_tels.end(), tel_id) != allowed_tels.end();
}