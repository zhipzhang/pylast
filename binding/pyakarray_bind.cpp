/**
 * @file pyakarray_bind.cpp
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Nanobind module exposing get_ak_array(EventSource) -> ak.Array.
 *        The returned array is a single unified nested structure where each
 *        optional data level is an IndexedOption field (None if absent).
 * @version 0.1
 * @date 2026-03-23
 *
 * @copyright Copyright (c) 2026
 */

#include "pylast/AkArrayBuilder.hh"
#include "EventSource.hh"

#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"

namespace nb = nanobind;

// ---------------------------------------------------------------------------
// snapshot_builder – adapted from the awkward-header pybind11 demo for nanobind
// ---------------------------------------------------------------------------
template <typename T>
nb::object snapshot_builder(const T& builder) {
    auto np = nb::module_::import_("numpy");
    auto ak = nb::module_::import_("awkward");

    // Determine required buffer sizes
    std::map<std::string, size_t> names_nbytes;
    builder.buffer_nbytes(names_nbytes);

    // Allocate NumPy uint8 buffers and collect raw pointers
    nb::dict py_container;
    std::map<std::string, void*> cpp_container;

    for (const auto& [name, nbytes] : names_nbytes) {
        nb::object arr = np.attr("empty")(nbytes, np.attr("dtype")("u1"));
        size_t ptr = nb::cast<size_t>(arr.attr("ctypes").attr("data"));
        py_container[nb::str(name.c_str())] = arr;
        cpp_container[name] = reinterpret_cast<void*>(ptr);
    }

    // Write builder contents into the allocated buffers
    builder.to_buffers(cpp_container);

    // Reconstruct an ak.Array from the form, length and buffers
    return ak.attr("from_buffers")(builder.form(), builder.length(), py_container);
}

// ---------------------------------------------------------------------------
// get_ak_array: iterate the source, fill all data levels, return snapshot
// ---------------------------------------------------------------------------
nb::object get_ak_array(EventSource& source) {
    ArrayEventBuilder aeb;
    for (auto& event : source) {
        aeb.append(event);
    }
    return snapshot_builder(aeb.builder);
}

NB_MODULE(_pylast_akarray, m) {
    m.doc() = "Awkward Array interface for pylast EventSources";

    m.def(
        "get_ak_array",
        &get_ak_array,
        nb::arg("source"),
        "Iterate *source* and return a single nested ak.Array.  "
        "Each event is one record; optional data levels (simulation, "
        "pointing, dl0, dl1, dl2) appear as None when absent."
    );
}
