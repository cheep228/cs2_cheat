#pragma once

#include <includes.h>
#include <functional>

namespace hell {
	void child( const char* str_id, hellvec2 size, std::function< void( ) > content, bool draw_label = true );
}
