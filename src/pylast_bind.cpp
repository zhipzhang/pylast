#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"
#include "EventSource.hh"
#include "SimtelEventSource.hh"

namespace nb = nanobind;


NB_MODULE(_pylast, m) {
    EventSource::bind(m);
    SimtelEventSource::bind(m);
}
