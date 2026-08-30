#include "window.h"

#include "../hell_gui.h"

using namespace ImGui;

void hell::begin_window( window_id id, hellvec2 size, hellcolor bg_color, float rounding, bool blur ) {
	hell::window::m_window_size = size;
	hell::window::m_window_rounding = rounding;
	hell::window::m_window_color = bg_color;
	hell::blur::m_wants_blur = blur && hell::blur::m_initialized;

	if ( hell::blur::m_wants_blur ) {
		hell::blur::blur_effect.begin_blur( );
	}

	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, rounding );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.f );

	ImGui::SetNextWindowSize( hell::window::m_window_size );
	ImGui::Begin( std::to_string( id ).c_str( ), nullptr, hell::window::m_flags );

	hell::begin( );

	hell::window::m_window_drawlist->AddRectFilled(
		hell::window::m_window_position,
		hell::window::m_window_position + hell::window::m_window_size,
		bg_color,
		rounding
	);
}

bool hell::is_position_in_illegal( hellvec2 pos ) {
	bool is_illegal = false;

	for ( auto& illegal_rect : hell::window::m_illegal_drag_rects ) {
		ImRect rect = illegal_rect.second;

		if ( rect.Contains( pos ) ) {
			is_illegal =  true;
		}
	}
	hell::window::m_illegal_drag_rects.clear( );
	return is_illegal;
}

void hell::end_window( ) {
	hell::end( );

	ImGui::End( );

	ImGui::PopStyleVar( 2 );
}
