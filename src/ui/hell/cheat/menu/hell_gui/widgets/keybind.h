#pragma once

#include <context.h>
#include <cheat/input.h>

namespace hell {
	static ImVec2 keybind_window_pos = ImVec2(100, 100); // default pos
	static bool dragging = false;
	static ImVec2 drag_offset;
	void render_keybind_list(const kb_map_t& widget_keybinds);
    keybind_t create_keybind( ImGuiID id, fnv1a_t holder_id, e_keybind_type type );
	bool keybind(ImGuiID id, keybind_t& keybind, fnv1a_t holder_id, bool value_changed);
    void keybind_handler( const std::string& id_string, hellvec2 open_pos, fnv1a_t holder_id, e_keybind_type keybind_type, bool value_changed = false );
}