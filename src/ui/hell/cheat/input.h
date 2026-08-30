#pragma once

// Shim for the quint tree's cheat/input.h. The class is the original one - it
// was already self-contained apart from e_key_state - with the WndProc feeding
// it wired to this project's own hook in core/hooks.cpp.

#include <includes.h>
#include "config/vars.h"

class c_input {
private:
    std::array<e_key_state, 256 + 5> m_key_states = {};
public:
    void wnd_proc(UINT msg, WPARAM wparam, LPARAM lparam) {
        int key = -1;
        bool down = false;

        switch (msg) {
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: key = VK_LBUTTON; down = true;  break;
        case WM_LBUTTONUP:                          key = VK_LBUTTON; down = false; break;
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: key = VK_RBUTTON; down = true;  break;
        case WM_RBUTTONUP:                          key = VK_RBUTTON; down = false; break;
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: key = VK_MBUTTON; down = true;  break;
        case WM_MBUTTONUP:                          key = VK_MBUTTON; down = false; break;
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
            key = (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2; down = true; break;
        case WM_XBUTTONUP:
            key = (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2; down = false; break;
        case WM_KEYDOWN: case WM_SYSKEYDOWN: key = static_cast<int>(wparam); down = true;  break;
        case WM_KEYUP:   case WM_SYSKEYUP:   key = static_cast<int>(wparam); down = false; break;
        default: return;
        }

        if (key < 0 || key >= static_cast<int>(m_key_states.size()))
            return;

        if (down)
            m_key_states[key] = e_key_state::down;
        else if (m_key_states[key] == e_key_state::down)
            m_key_states[key] = e_key_state::released;
        else
            m_key_states[key] = e_key_state::up;
    }

    bool key_down(int btn) {
        if (btn < 0 || btn >= static_cast<int>(m_key_states.size()))
            return false;
        return m_key_states[btn] == e_key_state::down;
    }

    bool key_released(int btn) {
        if (btn < 0 || btn >= static_cast<int>(m_key_states.size()))
            return false;
        if (m_key_states[btn] == e_key_state::released) {
            m_key_states[btn] = e_key_state::up;
            return true;
        }
        return false;
    }
};
inline auto g_input = std::make_unique<c_input>();
