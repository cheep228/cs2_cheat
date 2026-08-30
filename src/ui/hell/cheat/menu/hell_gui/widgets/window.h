#pragma once

#include <includes.h>

typedef unsigned int window_id;

namespace hell {
	void begin_window( window_id id, hellvec2 size, hellcolor bg_color, float rounding = 0.f, bool blur = true );
	bool is_position_in_illegal( hellvec2 pos );
	void end_window( );
}