#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"
#include "Pet.hh"
#include "increment.hh"

namespace nb = nanobind;


NB_MODULE(_nanobind_example, m) {
    nb::class_<Pet>(m, "Pet")
        .def(nb::init<std::string, int>())
        .def_rw("name", &Pet::name)
        .def_rw("age", &Pet::age);
    nb::class_<Dog, Pet>(m, "Dog")
        .def(nb::init<std::string, int>())
        .def("bark", &Dog::bark);
    m.def("increment", &increment);
}

