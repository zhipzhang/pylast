#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"
#include "Basebind.hh"
#include "EventSource.hh"

namespace nb = nanobind;


NB_MODULE(_pylast, m) {
    Basebind::register_all(m);
    EventSource::bind(m);
}
