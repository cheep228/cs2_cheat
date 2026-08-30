#pragma once

#include <includes.h>

namespace hell {
	bool begin_combo_popup( ImGuiID id, ImRect bb );
	bool begin_combo( const char* label, const char* curr_val, bool draw_label = true );
	void end_combo( );
	bool selectable( const char* label, bool selected );
	bool combo( const char* label, int& selected_item, const char* items[ ], int count, bool draw_label = true );
	bool multi_combo( const char* label, const char* items[ ], std::vector<bool>& v, int size, bool draw_label = true );
}