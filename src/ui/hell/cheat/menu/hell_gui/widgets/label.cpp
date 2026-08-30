#include "label.h"

using namespace ImGui;

void hell::label( const char* text, hellcolor color ) {
	ImGui::TextColored( color, text );
}

void hell::label( ImFont* font, const char* text, hellcolor color ) {
	ImGui::PushFont( font );
	hell::label( text, color );
	ImGui::PopFont( );
}

void hell::label_shadow( hellvec2 pos, ImFont* font, const char* text, hellcolor color ) {
	ImDrawList* drawList = ImGui::GetWindowDrawList( );
	ImGui::PushFont( font );

	int rings = 10; // how many steps outward
	int segments = 8; // how many points around each ring
	float max_radius = 12.0f;
	int base_alpha = 20;

	for ( int r = 1; r <= rings; ++r ) {
		float t = r / (float)rings;
		float radius = t * max_radius;
		float fade = powf( 1.0f - t, 2.0f );
		int alpha = static_cast<int>( base_alpha * fade );
		hellcolor shadow_col = hellcolor( 0, 0, 0, alpha );

		for ( int s = 0; s < segments; ++s ) {
			float angle = ( 2.0f * IM_PI * s ) / segments;
			float dx = cosf( angle ) * radius;
			float dy = sinf( angle ) * radius;
			drawList->AddText( font, font->LegacySize, { pos.x + dx, pos.y + dy }, shadow_col, text );
		}
	}

	drawList->AddText( font, font->LegacySize, pos, color, text );
	ImGui::PopFont( );
}
