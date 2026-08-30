#pragma once

#include <includes.h>
#include "continue_line.h"
#include <cheat/config/vars.h>

namespace hell {
	enum e_button_alignment : unsigned int {
		button_alignment_center = 0,
		button_alignment_left,
		button_alignment_right
	};

	struct button_data_t {
		hellcolor m_fg_normal = { 255, 255, 255, 100 };
		hellcolor m_fg_hovered = { 255, 255, 255, 180 };
		hellcolor m_fg_pressed = { 255, 255, 255, 255 };
		hellcolor m_bg_normal = { 0, 0, 0, 0 };
		hellcolor m_bg_hovered = { 0, 0, 0, 0 };
		hellcolor m_bg_pressed = { 0, 0, 0, 0 };
		float m_animation_speed = 5.f;
		float m_frame_rounding = 2.f;
		int m_custom_id = 0;

		e_button_alignment m_text_alignment = e_button_alignment::button_alignment_center;

		button_data_t( ) = default;

		button_data_t( hellcolor fg_normal, hellcolor fg_hovered, hellcolor bg_normal, hellcolor bg_hovered ) : 
			m_fg_normal( fg_normal ), m_fg_hovered( fg_hovered ), m_bg_normal( bg_normal ), m_bg_hovered( bg_hovered ) 
		{ }

		button_data_t( hellcolor fg_normal, hellcolor fg_hovered, hellcolor fg_pressed, hellcolor bg_normal, hellcolor bg_hovered, hellcolor bg_pressed ) :
			m_fg_normal( fg_normal ), m_fg_hovered( fg_hovered ), m_fg_pressed( fg_pressed ), m_bg_normal( bg_normal ), m_bg_hovered( bg_hovered ), m_bg_pressed( bg_pressed ) {
		}
	};
	bool button( const char* label, hellvec2 size = { 0.f, 0.f }, button_data_t data = { } );
	bool button_pos( const char* label, hellvec2 size = { 0.f, 0.f }, hellvec2 pos = { 0.f, 0.f }, button_data_t data = { } );

	// Usage: hell::button_array<BUTTON_COUNT>( ... );
	// Return: { Any button pressed, Index of pressed button >
	template <int T>
	inline std::pair< bool, int > button_array( ImGuiID id, std::array<const char*, T> button_labels, bool inline_buttons, button_data_t& selected_data, button_data_t& unselected_data, bool underline = false, hellcolor underline_color = GET_VAR(hellcolor, MISC_PATH(m_accent_color)) ) {
		static std::unordered_map< ImGuiID, int > s_pressed_buttons;
		
		bool button_pressed = false;

		auto window = ImGui::GetCurrentWindow( );

		for ( int i = 0; i < T; i++ ) {
			const char* button_label = button_labels[ i ];
			if ( hell::button( button_label, { 0.f, 0.f }, ( i == s_pressed_buttons[ id ] ) ? selected_data : unselected_data ) ) {
				s_pressed_buttons[ id ] = i;
				button_pressed = true;
			}

			if ( underline && i == s_pressed_buttons[ id ] ) {
				ImVec2 min = ImGui::GetItemRectMin( );
				ImVec2 max = ImGui::GetItemRectMax( );
				ImGui::GetWindowDrawList( )->AddLine(
					{ min.x, max.y }, { max.x, max.y },
					underline_color, 1.0f
				);
			}

			if ( i != T - 1 && inline_buttons )
				hell::continue_line( );
		}

		return { button_pressed, s_pressed_buttons[ id ] };
	}
}