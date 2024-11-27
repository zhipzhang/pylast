/**
 * @file Basebind.hh
 * @brief Base class providing essential methods for binding classes with nanobind
 */

#pragma once

#include <nanobind/nanobind.h>
#include <string>
#include <vector>

namespace nb = nanobind;

class Basebind {
public:
    Basebind() = default;
    virtual ~Basebind() = default;

    /**
     * @brief Virtual method that must be implemented by derived classes to define their nanobind bindings
     * @param m The nanobind module to bind to
     */
    virtual void bind(nb::module_& m) = 0;
    virtual const std::string print() const = 0;
    static void register_all(nb::module_& m) {
        for (auto& func : get_registry()) {
            func(m);
        }
    }

protected:

    using BindFunction = void(*)(nb::module_&);
    static void add_bind_function(BindFunction func) {
        get_registry().emplace_back(func);
    }

private:
    static std::vector<BindFunction>& get_registry() {
        static std::vector<BindFunction> registry;
        return registry;
    }
};

// Auto register bind
template <typename Derived>
class AutoRegisterBind : public Basebind {
public:
    static void static_bind(nb::module_& m) {
        Derived().bind(m);
    }

protected:
    AutoRegisterBind() {
        Basebind::add_bind_function(&Derived::static_bind);
    }
};
