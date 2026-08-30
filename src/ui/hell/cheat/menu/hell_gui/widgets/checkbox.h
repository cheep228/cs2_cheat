#pragma once

#include <context.h>
#include <functional>
#include <sdk/datatypes/fnv1a.h>

namespace hell {
	bool checkbox_passthrough( const char* label, bool* v, int custom_id = 0 );
	bool checkbox( const char* label, std::variant<fnv1a_t, bool*> v, std::function<void( )> content = 0, int custom_id = 0, bool passthrough = false );
	float calc_checkbox_size( );
}