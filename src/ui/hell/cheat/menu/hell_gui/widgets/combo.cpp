#include "combo.h"

#include <cheat/menu/animation/anim.h>
#include <cheat/menu/hell_gui/colors.h>
#include <cheat/menu/hell_gui/hell_gui.h>

using namespace ImGui;

static float CalcMaxPopupHeightFromItemCount( int iItemCount ) {
	ImGuiContext& g = *GImGui;
	if ( iItemCount <= 0 )
		return FLT_MAX;
	return ( g.FontSize + g.Style.ItemSpacing.y ) * iItemCount - g.Style.ItemSpacing.y + ( g.Style.WindowPadding.y * 2 );
}

bool hell::begin_combo_popup( ImGuiID id, ImRect bb ) {
	ImGuiContext& g = *GImGui;
	if ( !IsPopupOpen( id, ImGuiPopupFlags_None ) ) {
		g.NextWindowData.ClearFlags( );
		return false;
	}

	float w = 200.0f; // fixed size, since i think it'd look better
	SetNextWindowSize( hellvec2( w, 0.0f ) );

	int popup_max_height_in_items = 8;

	hellvec2 constraint_min( 0.0f, 0.0f ), constraint_max( FLT_MAX, FLT_MAX );
	SetNextWindowSizeConstraints( constraint_min, constraint_max );

	char name[ 16 ];
	ImFormatString( name, IM_ARRAYSIZE( name ), "##Combo_%02d", g.BeginPopupStack.Size );

	if ( ImGuiWindow* popup_window = FindWindowByName( name ) ) {
		if ( popup_window->WasActive ) {
			hellvec2 size_expected = CalcWindowNextAutoFitSize( popup_window );
			popup_window->AutoPosLastDirection = ImGuiDir_Left;
			ImRect r_outer = GetPopupAllowedExtentRect( popup_window );
			hellvec2 pos = FindBestWindowPosForPopupEx( bb.GetBL( ), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox );
			SetNextWindowPos( pos + hellvec2( 0, 4 ) );
		}
	}

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
	PushStyleVar( ImGuiStyleVar_WindowPadding, hellvec2( 5, 5 ) );

	PushStyleVar( ImGuiStyleVar_PopupRounding, 5.f );

	PushStyleColor( ImGuiCol_PopupBg, hellcolor( 40, 40, 40, 240 ).Value );
	PushStyleColor( ImGuiCol_Border, hellcolor( 45, 45, 45, 150 ).Value );

	bool ret = Begin( name, NULL, window_flags );

	PopStyleColor( 2 );
	PopStyleVar( 2 );

	if ( !ret ) {
		EndPopup( );
		return false;
	}
	return true;
}

bool hell::begin_combo( const char* label, const char* current_value, bool draw_label ) {
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow( );

	g.NextWindowData.ClearFlags( );
	if ( window->SkipItems )
		return false;

	const ImGuiStyle& im_style = g.Style;
	const ImGuiID im_id = window->GetID( label );

	hellvec2 label_size( 0.0f, 0.0f );
	const char* render_label_text_pointer = label;

	if ( draw_label ) {
		label_size = CalcTextSize( label, NULL, true );
		if ( label_size.x == 0.0f ) {
			draw_label = false;
			render_label_text_pointer = NULL;
		}
	} else {
		render_label_text_pointer = NULL;
	}

	const float frame_height = GetFrameHeight( ) - 2.f;

	float width_for_combo_and_picker = GetContentRegionAvail( ).x;
	if ( draw_label ) {
		width_for_combo_and_picker -= ( label_size.x + im_style.ItemInnerSpacing.x );
		width_for_combo_and_picker = ImMax( 1.0f, width_for_combo_and_picker );
	}

	float combo_actual_width = hell::config::m_widget_width;

	hellvec2 combo_frame_position = window->DC.CursorPos;
	if ( draw_label ) {
		combo_frame_position.x = window->WorkRect.Max.x - combo_actual_width;
	} else {
		combo_frame_position.y -= 1.5f;
	}

	const ImRect im_frame_bounding_box(
		combo_frame_position,
		combo_frame_position + hellvec2( combo_actual_width, frame_height )
	);

	const ImRect im_total_bounding_box(
		window->DC.CursorPos,
		window->DC.CursorPos + hellvec2(
			( draw_label ? label_size.x + im_style.ItemInnerSpacing.x : 0.0f ) + width_for_combo_and_picker,
			frame_height
		)
	);

	ItemSize( im_total_bounding_box, im_style.FramePadding.y );
	if ( !ItemAdd( im_total_bounding_box, im_id, &im_frame_bounding_box ) )
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior( im_frame_bounding_box, im_id, &hovered, &held );
	const ImGuiID im_popup_id = ImHashStr( xx( "##ComboPopup" ), 0, im_id );
	bool popup_open = IsPopupOpen( im_popup_id, ImGuiPopupFlags_None );

	if ( pressed && !popup_open ) {
		OpenPopupEx( im_popup_id, ImGuiPopupFlags_None );
		popup_open = true;
	}

	hellcolor bg_color = { 45, 45, 45, 120 };
	constexpr float bg_rect_radius = 2.f;
	window->DrawList->AddRectFilled( im_frame_bounding_box.Min, im_frame_bounding_box.Max, bg_color, bg_rect_radius );
	window->DrawList->AddRect( im_frame_bounding_box.Min, im_frame_bounding_box.Max, hellcolor( 80, 80, 80, 50 ), bg_rect_radius );

	float* interpolated_label_alpha = ImGui::GetStateStorage( )->GetFloatRef( im_id + 1, 0.f );
	float target_alpha_selected = popup_open ? 1.f : hell::colors::m_text_unhovered.Value.w;
	*interpolated_label_alpha = ImLerp( *interpolated_label_alpha, target_alpha_selected, ImGui::GetIO( ).DeltaTime * 8.f );
	*interpolated_label_alpha = ImClamp( *interpolated_label_alpha, 0.0f, 1.0f );

	if ( draw_label && render_label_text_pointer ) {
		ImVec4 base_color_rgb_values;
		if ( popup_open )
			base_color_rgb_values = hell::colors::m_text_hovered.Value;
		else
			base_color_rgb_values = hell::colors::m_text_unhovered.Value;

		ImVec4 final_label_color = ImVec4( base_color_rgb_values.x, base_color_rgb_values.y, base_color_rgb_values.z, *interpolated_label_alpha );

		hellvec2 label_render_position = hellvec2(
			window->DC.CursorPos.x,
			im_frame_bounding_box.Min.y + ( frame_height - label_size.y ) * 0.5f
		);
		window->DrawList->AddText( label_render_position, ColorConvertFloat4ToU32( final_label_color ), render_label_text_pointer );
	}

	hellvec2 current_text_size = CalcTextSize( current_value );

	hellvec2 im_selected_text_position = hellvec2(
		im_frame_bounding_box.Min.x + im_style.FramePadding.x,
		( im_frame_bounding_box.Min.y + im_frame_bounding_box.Max.y ) * 0.5f - current_text_size.y * 0.5f
	);

	float selected_text_max_x = im_frame_bounding_box.Max.x - im_style.FramePadding.x;
	ImRect clip_rect_text( im_frame_bounding_box.Min.x + im_style.FramePadding.x, im_frame_bounding_box.Min.y, selected_text_max_x, im_frame_bounding_box.Max.y );

	window->DrawList->PushClipRect( clip_rect_text.Min, clip_rect_text.Max, true );
	window->DrawList->AddText( im_selected_text_position, hell::colors::m_text_selected, current_value );
	window->DrawList->PopClipRect( );

	{ // arrow
		hellcolor bg_color_rect_transparent = { 45, 45, 45, 0 };
		hellcolor bg_color_rect = { 45, 45, 45, 255 };
		float rect_width = 20.f;

		ImRect color_rect_right = { { im_frame_bounding_box.Max.x - rect_width, im_frame_bounding_box.Min.y }, { im_frame_bounding_box.Max.x, im_frame_bounding_box.Max.y } };
		ImRect color_rect_left = { { color_rect_right.Min.x - rect_width, color_rect_right.Min.y }, { color_rect_right.Min.x, color_rect_right.Max.y } };

		bool should_draw_bg = current_text_size.x >= combo_actual_width - ( color_rect_right.Max.x - color_rect_left.Min.x );

		if ( should_draw_bg ) {
			window->DrawList->AddRectFilled( color_rect_right.Min, color_rect_right.Max, bg_color_rect, 2.f, ImDrawFlags_RoundCornersRight );
			window->DrawList->AddRectFilledMultiColor( color_rect_left.Min, color_rect_left.Max, bg_color_rect_transparent, bg_color_rect, bg_color_rect, bg_color_rect_transparent );
		}

		hellvec2 arrow_pos = { color_rect_right.Min.x + rect_width * 0.5f - 5.f, color_rect_right.Min.y + ( color_rect_right.Max.y - color_rect_right.Min.y ) * 0.5f - 5.f };
		RenderArrow( window->DrawList, arrow_pos, hellcolor( 255, 255, 255, 180 ), popup_open ? ImGuiDir_Down : ImGuiDir_Up, 0.6f );
	}

	if ( popup_open )
		if ( hell::begin_combo_popup( im_popup_id, im_frame_bounding_box ) )
			return true;

	return false;
}


void hell::end_combo( ) {
	ImGuiContext& g = *GImGui;
	EndPopup( );
	g.BeginComboDepth--;
}

bool hell::selectable( const char* label, bool selected ) {
	ImGuiWindow* im_window = ImGui::GetCurrentWindow( );
	if ( im_window->SkipItems )
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& im_style = g.Style;

	hellvec2 im_label_size = CalcTextSize( label, NULL, true );
	hellvec2 im_avail_region = ImGui::GetContentRegionAvail( );

	hellvec2 im_item_size(
		im_avail_region.x,
		im_label_size.y
	);

	hellvec2 im_pos = im_window->DC.CursorPos;
	ItemSize( im_item_size, 0.0f );

	ImRect im_bb( im_pos, im_pos + im_item_size );
	if ( !ItemAdd( im_bb, im_window->GetID( label ) ) )
		return false;

	bool b_hovered, b_held;
	bool b_pressed = ButtonBehavior( im_bb, im_window->GetID( label ), &b_hovered, &b_held );

	ImDrawList* im_draw_list = im_window->DrawList;
	hellvec2 im_text_pos = { im_bb.Min.x, im_bb.GetCenter( ).y - ( im_label_size.y * 0.5f ) };

	std::string str_new_label( label );
	size_t pos = str_new_label.find( xx( "##" ) );

	if ( pos != std::string::npos ) {
		str_new_label = str_new_label.substr( 0, pos );
	}

	ImGuiID id = im_window->GetID( label );

	float* select_progress = ImGui::GetStateStorage( )->GetFloatRef( id, 0.0f );
	float* hover_progress = ImGui::GetStateStorage( )->GetFloatRef( id + 1, 0.0f );
	float* color_transition = ImGui::GetStateStorage( )->GetFloatRef( id + 2, 0.0f );

	*select_progress = ImSaturate( *select_progress + g.IO.DeltaTime * ( selected ? 15.0f : -15.0f ) );
	*hover_progress = ImSaturate( *hover_progress + g.IO.DeltaTime * ( b_hovered && !selected ? 15.0f : -15.0f ) );

	hellcolor normal_color = hell::colors::m_text_unhovered;
	hellcolor hover_color = hell::colors::m_text_hovered;
	hellcolor selected_color = hellcolor(140, 90, 190, 255);

	hellcolor target_color = normal_color;
	if ( selected )
		target_color = selected_color;
	else if ( b_hovered )
		target_color = hover_color;

	*color_transition = ImLerp( *color_transition, target_color.Value.w, g.IO.DeltaTime * 10.0f );
	target_color.Value.w = *color_transition;

	float x_offset = ImLerp( 0.0f, 3.3f, *select_progress ) + ImLerp( 0.0f, 1.5f, *hover_progress );
	im_text_pos.x += x_offset;

	im_draw_list->AddText( im_text_pos, target_color, str_new_label.c_str( ) );

	return b_pressed;
}

bool hell::combo( const char* label, int& selected_item, const char* items[ ], int count, bool draw_label ) {
	ImGuiID imID = GetID( label );

	if ( hell::begin_combo( label, items[ selected_item ], draw_label ) ) {
		for ( int i = 0; i < count; i++ ) {
			ImGuiID item_id = imID + i;
			PushID( item_id );
			if ( hell::selectable( items[ i ], selected_item == i ) ) {
				selected_item = i;
			}
			PopID( );
		}
		hell::end_combo( );
	}

	return true;
}

bool hell::multi_combo( const char* label, const char* items[ ], std::vector<bool>& v, int size, bool draw_label ) {
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if ( window->SkipItems )
		return false;

	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	std::string szBuffer;
	const hellvec2 vecLabelSize = ImGui::CalcTextSize( label );
	float flActiveWidth = ImGui::CalcItemWidth( ) - ( style.ItemInnerSpacing.x + ImGui::GetFrameHeight( ) ) - 10.f;

	for ( int i = 0; i < size; i++ ) {
		if ( v[ i ] ) {
			hellvec2 vecTextSize = ImGui::CalcTextSize( szBuffer.c_str( ) );

			if ( szBuffer.empty( ) )
				szBuffer.assign( items[ i ] );
			else
				szBuffer.append( xx( ", " ) ).append( items[ i ] );

			if ( vecTextSize.x > flActiveWidth )
				szBuffer.erase( szBuffer.find_last_of( xx( "," ) ) ).append( xx( "..." ) );
		}
	}

	if ( szBuffer.empty( ) )
		szBuffer.assign( xx( "None" ) );

	ImGuiID imID = ImGui::GetID( label );

	bool value_changed = false;
	if ( hell::begin_combo( label, szBuffer.c_str( ), draw_label ) ) {
		for ( int i = 0; i < size; i++ ) {
			if ( hell::selectable( items[ i ], v[ i ] ) ) {
				v[ i ] = !v[ i ];
				value_changed = true;
			}
		}

		hell::end_combo( );
	}

	return value_changed;
}
