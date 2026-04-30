#include "BaseTelContainer.hh"
#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/pair.h"
namespace nb = nanobind;

template<typename TelData>
void bind_basetelcontainer(nb::module_ &m) {
    nb::class_<BaseTelContainer<TelData>>(m, "BaseTelContainer")
        .def("__getitem__", [](BaseTelContainer<TelData> &container, int tel_id) -> TelData* {
            auto tel = container.get_tel(tel_id);
            if (!tel) {
                throw std::out_of_range("Telescope ID not found.");
            }
            return tel;
        }, nb::rv_policy::reference_internal)
        .def("keys", [](const BaseTelContainer<TelData> &container) {
            std::vector<int> keys;
            for (const auto &pair : container.tels) {
                keys.push_back(pair.first);
            }
            return keys;
        })
        .def("values", [](BaseTelContainer<TelData> &container) {
            std::vector<TelData*> values;
            for (auto &pair : container.tels) {
                values.push_back(pair.second.get());
            }
            return values;
        }, nb::rv_policy::reference_internal)
        .def("items", [](BaseTelContainer<TelData> &container) {
            std::vector<std::pair<int, TelData*>> items;
            for (auto &pair : container.tels) {
                items.emplace_back(pair.first, pair.second.get());
            }
            return items;
        }, nb::rv_policy::reference_internal)
        .def("__len__", [](const BaseTelContainer<TelData> &container) {
            return container.tels.size();
        })
        .def("__contains__", [](const BaseTelContainer<TelData> &container, int tel_id) {
            return container.tels.find(tel_id) != container.tels.end();
        })
        .def("__repr__", [](const BaseTelContainer<TelData> &container) {
            return "<BaseTelContainer with " + std::to_string(container.tels.size()) + " telescopes>";
        });
}