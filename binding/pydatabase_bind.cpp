#include "DatabaseWriter.hh"
#include "nanobind/nanobind.h"
#include "EventSource.hh"
#include "nanobind/nb_defs.h"
#include "nanobind/stl/string.h"




void bind_databasewriter(nanobind::module_ &m) {
    using namespace nanobind;

    class_<DatabaseWriter>(m, "DatabaseWriter")
        .def(init<const std::string&>(), arg("db_file"))
        .def("initialize", &DatabaseWriter::initialize)
        .def("writeEventData", &DatabaseWriter::writeEventData, arg("event_source"), arg("use_true") = false)
        .def("__call__", [](DatabaseWriter &self, EventSource &event_source, bool use_true) {
            self.writeEventData(event_source, use_true);
        })
        .def_ro("db_file", &DatabaseWriter::db_file_)
        .def("clearTables", &DatabaseWriter::clearTables)
        .def_ro("db_file", &DatabaseWriter::db_file_)
        .def("db_ptr", [](DatabaseWriter &self) {
            return capsule(self.db_ptr().get(), "duckdb::DuckDB");
        }, 
             "Get the DuckDB pointer for this DatabaseWriter")
        .def("__repr__", [](const DatabaseWriter& self) {
            return ("DatabaseWriter: " + self.db_file_);
        });
}

NB_MODULE(_pylast_databasewriter, m)
{
    bind_databasewriter(m);
}