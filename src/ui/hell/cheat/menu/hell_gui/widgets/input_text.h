#pragma once

#include <includes.h>

namespace hell {
	bool input_text( const char* label, char* buf, size_t buf_size, const char* placeholder = nullptr, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None, float width = -1.f, int custom_id = 0 );
	bool input_text_multiline( const char* label, char* buf, size_t buf_size, const char* placeholder = nullptr, hellvec2 size = { 0.f, 0.f }, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None, int custom_id = 0 );
}