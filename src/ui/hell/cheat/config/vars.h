#pragma once

// Shim for the quint tree's vars.h.
//
// The original declares every setting the whole cheat has (27 KB of ADD_VAR).
// This project keeps its settings in core/globals.h instead, and the tabs bind
// to those through the widgets' _passthrough overloads. So only what hell_gui
// itself reaches for is declared here:
//
//   MISC_PATH( m_accent_color )        - the menu accent, read by most widgets
//   CONFIG_PATH( m_widget_keybinds )   - the keybind list drawn on screen
//   e_key_state / e_key_mode / e_keybind_type / keybind_t / kb_map_t

#include "config_system.h"

enum class e_key_state : int {
    none = 0,
    down,
    up,
    released
};

enum e_key_mode : unsigned int {
    key_mode_toggle = 0,
    key_mode_hold
};

enum e_keybind_type : unsigned int {
    keybind_type_unset = 0,
    keybind_type_checkbox,
    keybind_type_slider_int,
    keybind_type_slider_float
};

struct keybind_t;
typedef std::unordered_map<ImGuiID, std::vector<keybind_t>> kb_map_t;

struct keybind_t {
    e_key_mode m_mode = e_key_mode::key_mode_hold;
    fnv1a_t m_holder_id = static_cast<fnv1a_t>(-1);
    int m_key = -1;

    e_keybind_type m_keybind_type = e_keybind_type::keybind_type_unset;

    std::variant<bool, int, float> m_default_val;
    std::variant<std::monostate, bool, int, float> m_override_val;

    bool m_is_on_mode = true;
    bool m_keybind_active = false;
    bool m_on_mode_activated = false;

    keybind_t() = default;

    keybind_t(ImGuiID widget_id, fnv1a_t holder_id, int key, e_keybind_type kb_type, std::variant<bool, int, float> default_val)
        : m_mode(e_key_mode::key_mode_hold), m_holder_id(holder_id), m_key(key), m_keybind_type(kb_type), m_default_val(default_val), m_override_val(std::monostate{}), m_is_on_mode(true), m_keybind_active(false), m_on_mode_activated(false) {
        (void)widget_id;
    }
};

class c_vars {
public:
    struct {
        ADD_VAR(hellcolor, m_accent_color, hellcolor(122, 92, 255, 255));
        ADD_VAR(bool, m_menu_click_sounds, false);
    } m_misc;

    struct {
        ADD_VAR(kb_map_t, m_widget_keybinds, kb_map_t{});
    } m_config;
};
inline auto g_vars = std::make_unique<c_vars>();

#define MISC_PATH( variable ) g_vars->m_misc.variable
#define CONFIG_PATH( variable ) g_vars->m_config.variable
