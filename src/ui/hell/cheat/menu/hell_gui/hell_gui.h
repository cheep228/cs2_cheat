#pragma once  

#include <includes.h>  
#include <functional>

#include <utils/tight_array.h>

#include "blur.h"  
#include "drawlist.h"  

#include "widgets/window.h"  
#include "widgets/checkbox.h"  
#include "widgets/label.h"  
#include "widgets/button.h"  
#include "widgets/continue_line.h"  
#include "widgets/color_picker.h"  
#include "widgets/child.h"  
#include "widgets/slider.h"  
#include "widgets/combo.h"  
#include "widgets/input_text.h"  

namespace hell {
	namespace window {  
		inline ImDrawList* m_window_drawlist;  
		inline ImDrawList* m_bg_drawlist;  
		inline ImDrawList* m_fg_drawlist;  

		inline hellvec2 m_window_position;  
		inline hellvec2 m_window_size;  

		inline std::unordered_map<ImGuiID, ImRect> m_illegal_drag_rects;

		inline hellcolor m_window_color;
		
		inline ImGuiWindowFlags m_flags;

		inline ImRect m_bb;  

		inline float m_window_rounding = 0.f;
		inline float m_footer_height = 0.f;  

		inline void update( ) {  
			m_window_drawlist = ImGui::GetWindowDrawList( );  
			m_bg_drawlist = ImGui::GetBackgroundDrawList( );  
			m_fg_drawlist = ImGui::GetForegroundDrawList( );  

			m_window_position = ImGui::GetWindowPos( );  

			m_bb = { m_window_position, m_window_position + m_window_size };  
		}  
	};  

	namespace blur {  
		inline bool m_wants_blur = false;  
		inline bool m_initialized = false;  

		inline DX11BlurEffect blur_effect;  

		inline void apply( ) {  
			if ( !m_wants_blur || !m_initialized )  
				return;

			if ( !g_interfaces || !g_interfaces->m_device || !g_interfaces->m_device_context )
				return;

			blur_effect.apply_blur( hell::window::m_window_drawlist, hell::window::m_window_position, hell::window::m_window_size, 0.f, hell::window::m_window_rounding );  
		}  

		inline void end( ) {
			if ( !m_wants_blur || !m_initialized )
				return;

			if ( !g_interfaces || !g_interfaces->m_device || !g_interfaces->m_device_context )
				return;

			blur_effect.end_blur( );  
		}  
	}

	namespace config {
		inline float m_widget_width = 120.f;
	}

	void begin( );  
	void end( );  

	void set_cursor_pos_relative_y( float amount = 14.f );
	hellvec2 calculate_column_size( int count );
	float get_available_height( );
	float calculate_child_height( std::function<void( )> content );
}