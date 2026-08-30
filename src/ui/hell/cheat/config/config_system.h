#pragma once

// Shim for the quint tree's config_system.h.
//
// The original is a std::any store keyed by an fnv1a hash, plus json save/load.
// hell_gui only ever reads from it, through GET_VAR, so the store is kept and
// the json persistence is dropped - this project already has its own config in
// core/config_io.h and does not need a second one.

#include <includes.h>
#include <any>
#include <sdk/datatypes/fnv1a.h>

struct config_var_t {
    std::any m_val;
    fnv1a_t  m_name_hash = 0;
    fnv1a_t  m_data_type_hash = 0;

    template <typename t>
    t& as() {
        return *(t*)(std::any_cast<t>(&m_val));
    }
};

class c_config_system {
private:
    std::unordered_map<fnv1a_t, config_var_t> m_config_variables;
public:
    template <typename t>
    t& get(fnv1a_t path) {
        auto it = m_config_variables.find(path);
        if (it == m_config_variables.end() || !it->second.m_val.has_value())
            it = m_config_variables.insert_or_assign(path, config_var_t{ std::make_any<t>(t{}), path, 0 }).first;
        return it->second.as<t>();
    }

    template <typename t>
    fnv1a_t add_variable(t value, fnv1a_t name_hash, fnv1a_t data_type_hash) {
        return (m_config_variables[name_hash] = { std::make_any<t>(value), name_hash, data_type_hash }, name_hash);
    }
};
inline auto g_config_system = std::make_unique<c_config_system>();

#define ADD_VAR( type, name, default_val ) fnv1a_t name = g_config_system->add_variable<type>( default_val, fnv_hash( #name ), fnv_hash( #type ) )
#define GET_VAR( type, path ) g_config_system->get<type>( path )
