#include "hell_gui.h"

void hell::begin( ) {
	window::update( );
	blur::apply( );
}

void hell::end( ) {
	blur::end( );
}

void hell::set_cursor_pos_relative_y( float amount ) {
	ImGui::SetCursorPosY( ImGui::GetCursorPosY( ) + amount );
}

hellvec2 hell::calculate_column_size( int count ) {
	float column_count = (float)count;

	hellvec2 avail = ImGui::GetContentRegionAvail( );
	hellvec2 window_padding = ImGui::GetStyle( ).WindowPadding;

	float available_width = avail.x - window_padding.x * 2.f;

	return { ( available_width - ( window_padding.x * ( column_count - 1.f ) ) ) / column_count, avail.y - window::m_footer_height };
}

float hell::get_available_height( ) {
	return ImGui::GetContentRegionAvail( ).y - window::m_footer_height;
}

float hell::calculate_child_height( std::function<void( )> content ) {
	static int measure_counter = 0;
	char id_buf[ 64 ];
	sprintf_s( id_buf, "##measure_%d", measure_counter++ );
	
	ImGuiWindow* parent_window = ImGui::GetCurrentWindow( );
	hellvec2 cursor_pos_before = ImGui::GetCursorPos( );
	
	float width = ImGui::GetContentRegionAvail( ).x;
	ImGui::BeginChild( id_buf, { width, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration );
	ImGui::SetCursorPosY( ImGui::GetCursorPosY( ) + 8.f );
	ImGui::Dummy( { 0.f, 0.f } );
	content( );
	ImGuiWindow* child_window = ImGui::GetCurrentWindow( );
	float height = 0.f;
	if ( child_window && child_window != parent_window ) {
		hellvec2 window_padding = ImGui::GetStyle( ).WindowPadding;
		height = ( child_window->DC.CursorMaxPos.y - child_window->DC.CursorStartPos.y ) + window_padding.y * 2.f + 8.f;
	}
	ImGui::EndChild( );
	
	ImGui::SetCursorPos( cursor_pos_before );
	
	return height > 0.f ? height : 200.f;
}
