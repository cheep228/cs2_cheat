#pragma once

// Shim for the quint source tree's own <context.h>.
//
// hell_gui touches exactly two things from it: g_interfaces->m_device and
// g_interfaces->m_device_context, both used by the DX11 blur. The original
// header drags in their whole SDK (pawn, controller, usercmd), none of which a
// menu needs, so this maps those two pointers onto the ones this project
// already owns in core/globals.h and stops there.

#include <includes.h>

#include "core/globals.h"

struct hell_interfaces_t {
    ID3D11Device*        m_device         = nullptr;
    ID3D11DeviceContext* m_device_context = nullptr;

    // Read fresh every access: the hook recreates the device on resize.
    void sync() {
        m_device = g::device;
        m_device_context = g::context;
    }
};

inline hell_interfaces_t* hell_get_interfaces() {
    static hell_interfaces_t interfaces;
    interfaces.sync();
    return &interfaces;
}

#define g_interfaces (hell_get_interfaces())
