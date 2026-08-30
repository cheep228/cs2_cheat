#pragma once

#include <includes.h>

namespace hell {
	void label( const char* text, hellcolor color = { 255, 255, 255, 255 } );
	void label( ImFont* font, const char* text, hellcolor color );
	void label_shadow( hellvec2 pos, ImFont* font, const char* text, hellcolor color );
}