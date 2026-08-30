#include "visual_preview.h"
#include <cheat/features/visuals/grenade.h>
#include <cheat/config/vars.h>
#include <sdk/constants.h>
#include <algorithm>
#include <string>
#include <vector>

static icon_data_t get_weapon_icon_by_name_preview(const std::string& weapon_name)
{
	if (weapon_name.empty())
		return {};

	std::string clean_name = weapon_name;
	if (clean_name.length() >= 7 && clean_name.substr(0, 7) == "weapon_")
		clean_name.erase(0, 7);

	std::vector<std::string> paths_to_try;
	paths_to_try.push_back("icons/equipment/" + clean_name);

	std::string name_without_numbers = clean_name;
	name_without_numbers.erase(std::remove_if(name_without_numbers.begin(), name_without_numbers.end(), ::isdigit), name_without_numbers.end());
	if (!name_without_numbers.empty() && name_without_numbers != clean_name)
		paths_to_try.push_back("icons/equipment/" + name_without_numbers);

	if (clean_name.find("_") != std::string::npos)
	{
		std::string name_without_underscore = clean_name;
		name_without_underscore.erase(std::remove(name_without_underscore.begin(), name_without_underscore.end(), '_'), name_without_underscore.end());
		if (!name_without_underscore.empty() && name_without_underscore != clean_name)
			paths_to_try.push_back("icons/equipment/" + name_without_underscore);
	}

	for (const auto& path : paths_to_try)
	{
		auto icon_data = get_panorama_texture(path);
		if (icon_data.texture_view && icon_data.width > 0 && icon_data.height > 0)
			return icon_data;
	}

	return {};
}

static icon_data_t get_weapon_icon_preview(uint16_t weapon_index)
{
	std::string weapon_name;
	switch (weapon_index)
	{
	case WEAPON_AK_47: weapon_name = "ak47"; break;
	case WEAPON_M4A4: weapon_name = "m4a4"; break;
	case WEAPON_M4A1_S: weapon_name = "m4a1_silencer"; break;
	case WEAPON_AUG: weapon_name = "aug"; break;
	case WEAPON_FAMAS: weapon_name = "famas"; break;
	case WEAPON_GALIL_AR: weapon_name = "galil"; break;
	case WEAPON_SG_553: weapon_name = "sg556"; break;
	case WEAPON_AWP: weapon_name = "awp"; break;
	case WEAPON_SSG_08: weapon_name = "ssg08"; break;
	case WEAPON_G3SG1: weapon_name = "g3sg1"; break;
	case WEAPON_SCAR_20: weapon_name = "scar20"; break;
	case WEAPON_P90: weapon_name = "p90"; break;
	case WEAPON_MP7: weapon_name = "mp7"; break;
	case WEAPON_MP9: weapon_name = "mp9"; break;
	case WEAPON_MP5_SD: weapon_name = "mp5sd"; break;
	case WEAPON_UMP_45: weapon_name = "ump45"; break;
	case WEAPON_MAC_10: weapon_name = "mac10"; break;
	case WEAPON_PP_BIZON: weapon_name = "bizon"; break;
	case WEAPON_M249: weapon_name = "m249"; break;
	case WEAPON_NEGEV: weapon_name = "negev"; break;
	case WEAPON_XM1014: weapon_name = "xm1014"; break;
	case WEAPON_SAWED_OFF: weapon_name = "sawedoff"; break;
	case WEAPON_MAG_7: weapon_name = "mag7"; break;
	case WEAPON_NOVA: weapon_name = "nova"; break;
	case WEAPON_DESERT_EAGLE: weapon_name = "deagle"; break;
	case WEAPON_R8_REVOLVER: weapon_name = "revolver"; break;
	case WEAPON_GLOCK_18: weapon_name = "glock"; break;
	case WEAPON_P2000: weapon_name = "p2000"; break;
	case WEAPON_USP_S: weapon_name = "usp_silencer"; break;
	case WEAPON_P250: weapon_name = "p250"; break;
	case WEAPON_FIVE_SEVEN: weapon_name = "fiveseven"; break;
	case WEAPON_TEC_9: weapon_name = "tec9"; break;
	case WEAPON_CZ75_AUTO: weapon_name = "cz75a"; break;
	case WEAPON_DUAL_BERETTAS: weapon_name = "elite"; break;
	case WEAPON_ZEUS_X27: weapon_name = "zeus"; break;
	default: return {};
	}
	
	return get_weapon_icon_by_name_preview("weapon_" + weapon_name);
}

const char* alignment_names[ ] = {
	xx( "Left" ),
	xx( "Center" ),
	xx( "Right" )
};

using namespace visual_preview;

static std::pair<bool, std::variant<c_visuals::bar_object_t, c_visuals::text_object_t, c_visuals::icon_object_t>> handle_preview_object(
	ImDrawList* draw_list,
	c_visuals::bb_t box_bb,
	e_preview_type preview_type,
	std::variant<c_visuals::bar_object_t, c_visuals::text_object_t, c_visuals::icon_object_t> obj
) {
	hellcolor hover_color_rect = { 120, 120, 120, 120 };

	auto fn_should_move = [ ]( ImRect bb ) -> bool {
		bool should_move = ImGui::IsMouseHoveringRect( bb.Min, bb.Max ) && ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseDown( ImGuiMouseButton_Left ) );
		if (should_move) {
			ImGuiContext& g = *ImGui::GetCurrentContext();
			if (g.MovingWindow != nullptr) {
				g.MovingWindow = nullptr;
				ImGui::ClearActiveID();
			}
			ImGui::SetNextFrameWantCaptureMouse(false);
			ImGui::GetIO().WantCaptureMouse = false;
		}
		return should_move;
		};

	switch ( preview_type ) {
	case e_preview_type::preview_type_bar:
	{
		auto& bar_obj = std::get<c_visuals::bar_object_t>( obj );

		c_visuals::bb_t bb = g_visuals->get_element_bb( box_bb, bar_obj.m_element_position, bar_obj.m_element_index, bar_obj.m_padding, bar_obj.m_size );
		bb.normalize( );
		ImRect im_bb = { bb.m_min, bb.m_max };
		im_bb.Expand( 2.f );

		bool should_move = fn_should_move( im_bb );
		if ( should_move ) {
			//draw_list->AddRectFilled( im_bb.Min, im_bb.Max, hover_color_rect, 2.f );

		}

		return { should_move, bar_obj };
	}

	case e_preview_type::preview_type_text:
	{
		auto& text_obj = std::get<c_visuals::text_object_t>( obj );

		c_visuals::bb_t bb = g_visuals->get_element_bb( box_bb, text_obj.m_element_position, text_obj.m_element_index, text_obj.m_padding, text_obj.m_size );
		bb.normalize( );
		ImRect im_bb = { bb.m_min, bb.m_max };
		im_bb.Expand( 2.f );

		bool should_move = fn_should_move( im_bb );
		if ( should_move ) {
			//draw_list->AddRectFilled( im_bb.Min, im_bb.Max, hover_color_rect, 2.f );

		}

		return { should_move, text_obj };
	}

	case e_preview_type::preview_type_icon:
	{
		auto& icon_obj = std::get<c_visuals::icon_object_t>( obj );

		c_visuals::bb_t bb = g_visuals->get_element_bb( box_bb, icon_obj.m_element_position, icon_obj.m_element_index, icon_obj.m_padding, (float)icon_obj.m_size );
		bb.normalize( );
		ImRect im_bb = { bb.m_min, bb.m_max };
		im_bb.Expand( (float)icon_obj.m_size * 0.5f + 2.f );

		bool should_move = fn_should_move( im_bb );
		if ( should_move ) {
			//draw_list->AddRectFilled( im_bb.Min, im_bb.Max, hover_color_rect, 2.f );

		}

		return { should_move, icon_obj };
	}
	}

	// no good, true so itll override alpha and we know something is wrong.
	return { true, obj };
}

static std::pair<c_visuals::e_element_bb_position, int> find_closest_anchor(
	const c_visuals::bb_t& anchor,
	const hellvec2& mouse_pos,
	ImDrawList* draw_list,
	int padding,
	int size
) {
	vec2_t v2_mouse = { mouse_pos.x, mouse_pos.y };

	vec2_t v2_best_pos{};
	float best_dist = FLT_MAX;
	int best_row = -1;
	int best_pos = -1;

	std::vector<std::pair<c_visuals::bb_t, std::pair<int, int>>> elements;
	const int max_rows = 8;

	for ( int pos = 0; pos < c_visuals::e_element_bb_position::max; pos++ ) {
		for ( int row = 1; row < max_rows; row++ ) {

			c_visuals::bb_t bb = g_visuals->get_element_bb( anchor, ( c_visuals::e_element_bb_position )pos, row, ( float )padding, ( float )size );

			//draw_list->AddRectFilled( bb.m_min, bb.m_max, hellcolor( 120, 120, 120, 80 ), 2.f );

			elements.emplace_back(
				bb,
				std::make_pair( pos, row )
			);
		}
	}

	for ( const auto& element : elements ) {
		const auto& bb = element.first;
		ImRect hover_area = { bb.m_min, bb.m_max };

		if ( hover_area.Contains( mouse_pos ) ) {
			return { ( c_visuals::e_element_bb_position )element.second.first, element.second.second };
		}

		const int pos = element.second.first;
		const int row = element.second.second;

		hellvec2 center = bb.get_center( );
		vec2_t v2_center = { center.x, center.y };
		float dist = v2_center.dist_to( v2_mouse );

		if ( dist < best_dist ) {
			best_dist = dist;
			v2_best_pos = v2_center;
			best_pos = pos;
			best_row = row;
		}
	}

	if ( best_pos == -1 || best_row == -1 )
		return { c_visuals::e_element_bb_position::top, 1 };

	return { static_cast< c_visuals::e_element_bb_position >( best_pos ), best_row };
}

static void handle_drag( c_visuals::bb_t& bb, int& element_position, int& element_index, int padding, int size, ImDrawList* draw_list ) {
	if ( ImGui::IsPopupOpen( ( ImGuiID )0, ImGuiPopupFlags_AnyPopupId ) )
		return;

	ImGui::SetNextFrameWantCaptureMouse(false);
	ImGui::GetIO().WantCaptureMouse = false;
	
	ImGuiContext& g = *ImGui::GetCurrentContext();
	if (g.MovingWindow != nullptr) {
		g.MovingWindow = nullptr;
		ImGui::ClearActiveID();
	}
	
	hellvec2 mouse_pos = ImGui::GetIO( ).MousePos;

	/*float bar_size = std::abs( ImGui::GetIO( ).DisplaySize.y - bb.m_max.y );
	c_visuals::bb_t preview_bb = { mouse_pos, { mouse_pos.x, mouse_pos.y + bar_size } };
	g_visuals->draw_bar( preview_bb, obj, 1.f, draw_list );*/

	auto closest_anchor_result = find_closest_anchor( bb, mouse_pos, draw_list, padding, size );

	int tries = 0;
	while ( s_occupied_positions[ { closest_anchor_result.first, closest_anchor_result.second } ] && tries <= 3 ) {
		closest_anchor_result = find_closest_anchor( bb, mouse_pos, draw_list, padding, size );
		tries++;
	}

	if ( tries > 3 )
		return;

	s_occupied_positions[ { ( c_visuals::e_element_bb_position )element_position, element_index } ] = false;

	element_position = closest_anchor_result.first;
	element_index = closest_anchor_result.second;

	s_occupied_positions[ { closest_anchor_result.first, closest_anchor_result.second } ] = true;
}

static void handle_text_popup( c_visuals::bb_t& anchor, c_visuals::text_object_t& obj ) {
	c_visuals::bb_t bb = g_visuals->get_element_bb( anchor, obj.m_element_position, obj.m_element_index, ( float )obj.m_padding, ( float )obj.m_size );
	ImRect im_bb = { bb.m_min, bb.m_max };
	im_bb.Expand( 2.f ); // so its easier to click ofc
	bool right_clicked = ImGui::IsMouseClicked( ImGuiMouseButton_Right );
	bool hovering_bb = ImGui::IsMouseHoveringRect( im_bb.Min, im_bb.Max );

	if ( hovering_bb && right_clicked ) {
		ImGui::OpenPopup( obj.m_text );
	}

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, hellvec2( 10, 10 ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 6.0f ); // rounded corners

	ImGui::PushStyleColor( ImGuiCol_PopupBg, hellcolor( 20, 20, 20, 255 ).Value ); // background
	ImGui::PushStyleColor( ImGuiCol_Border, hellcolor( 255, 255, 255, 15 ).Value ); // border color
	ImGui::SetNextWindowSize( { 230, -1 } );
	if ( ImGui::BeginPopup( obj.m_text ) ) {
		hell::slider_int_passthrough( xx( "Padding" ), &obj.m_padding, 1, 12 );
		hell::slider_int_passthrough( xx( "Size" ), &obj.m_size, 1, 12 );

		hell::combo( xx( "Font" ), ( int& )obj.m_font_type, font_names, IM_ARRAYSIZE( font_names ) );
		hell::combo( xx( "Align" ), ( int& )obj.m_text_alignment, alignment_names, IM_ARRAYSIZE( alignment_names ) );
		hell::combo( xx( "Shadow" ), ( int& )obj.m_text_shadow_type, shadow_names, IM_ARRAYSIZE( shadow_names ) );

		ImGui::EndPopup( );
	}

	ImGui::PopStyleColor( 2 );
	ImGui::PopStyleVar( 3 );
}

static void handle_health_bar_popup( c_visuals::bb_t& anchor, c_visuals::bar_object_t& obj ) {
	const char* id = xx( "HEALTH_BAR_POPUP " );
	
	c_visuals::bb_t bb = g_visuals->get_element_bb( anchor, obj.m_element_position, obj.m_element_index, ( float )obj.m_padding, obj.m_size );
	ImRect im_bb = { bb.m_min, bb.m_max };
	im_bb.Expand( 2.f );
	bool right_clicked = ImGui::IsMouseClicked( ImGuiMouseButton_Right );
	bool hovering_bb = ImGui::IsMouseHoveringRect( im_bb.Min, im_bb.Max );

	if ( hovering_bb && right_clicked ) {
		ImGui::OpenPopup( id );
	}

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, hellvec2( 10, 10 ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 6.0f );

	ImGui::PushStyleColor( ImGuiCol_PopupBg, hellcolor( 20, 20, 20, 255 ).Value );
	ImGui::PushStyleColor( ImGuiCol_Border, hellcolor( 255, 255, 255, 15 ).Value );
	ImGui::SetNextWindowSize( { 230, -1 } );
	if ( ImGui::BeginPopup( id ) ) {
		hell::slider_int_passthrough( xx( "Padding" ), &obj.m_padding, 1, 12 );
		hell::slider_int_passthrough( xx( "Size" ), &obj.m_size, 1, 12 );

		hell::color_picker( xx( "Value FG" ), obj.m_number_value_text.m_fg_color, 1, true );
		hell::color_picker( xx( "Value BG" ), obj.m_number_value_text.m_bg_color, 2, true );
		hell::checkbox_passthrough( xx( "Show Value" ), &obj.m_number_value_enabled );
		hell::combo( xx( "Value Font" ), ( int& )obj.m_number_value_text.m_font_type, font_names, IM_ARRAYSIZE( font_names ) );

	
		hell::color_picker(xx("Gradient Color"), GET_VAR(hellcolor, VISUALS_PATH(m_health_bar_gradient_color)), 1, true);
		hell::checkbox( xx( "Gradient" ), VISUALS_PATH( m_health_bar_gradient_enabled ) );

		ImGui::EndPopup( );
	}

	ImGui::PopStyleColor( 2 );
	ImGui::PopStyleVar( 3 );
}

static void handle_armor_bar_popup( c_visuals::bb_t& anchor, c_visuals::bar_object_t& obj ) {
	const char* id = xx( "ARMOR_BAR_POPUP " );
	
	c_visuals::bb_t bb = g_visuals->get_element_bb( anchor, obj.m_element_position, obj.m_element_index, ( float )obj.m_padding, obj.m_size );
	ImRect im_bb = { bb.m_min, bb.m_max };
	im_bb.Expand( 2.f );
	bool right_clicked = ImGui::IsMouseClicked( ImGuiMouseButton_Right );
	bool hovering_bb = ImGui::IsMouseHoveringRect( im_bb.Min, im_bb.Max );

	if ( hovering_bb && right_clicked ) {
		ImGui::OpenPopup( id );
	}

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, hellvec2( 10, 10 ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 6.0f );

	ImGui::PushStyleColor( ImGuiCol_PopupBg, hellcolor( 20, 20, 20, 255 ).Value );
	ImGui::PushStyleColor( ImGuiCol_Border, hellcolor( 255, 255, 255, 15 ).Value );
	ImGui::SetNextWindowSize( { 230, -1 } );
	if ( ImGui::BeginPopup( id ) ) {
		hell::slider_int_passthrough( xx( "Padding" ), &obj.m_padding, 1, 12 );
		hell::slider_int_passthrough( xx( "Size" ), &obj.m_size, 1, 12 );

		hell::color_picker( xx( "Value FG" ), obj.m_number_value_text.m_fg_color, 1, true );
		hell::color_picker( xx( "Value BG" ), obj.m_number_value_text.m_bg_color, 2, true );
		hell::checkbox_passthrough( xx( "Show Value" ), &obj.m_number_value_enabled );
		hell::combo( xx( "Value Font" ), ( int& )obj.m_number_value_text.m_font_type, font_names, IM_ARRAYSIZE( font_names ) );

	

		hell::color_picker(xx("Gradient Color"), GET_VAR(hellcolor, VISUALS_PATH(m_armor_bar_gradient_color)), 1, true);
		hell::checkbox( xx( "Gradient" ), VISUALS_PATH( m_armor_bar_gradient_enabled ) );

		ImGui::EndPopup( );
	}

	ImGui::PopStyleColor( 2 );
	ImGui::PopStyleVar( 3 );
}

static bool& get_popup_just_closed_flag() {
	static bool popup_just_closed = false;
	return popup_just_closed;
}

static void handle_icon_popup( const char* id, c_visuals::bb_t& anchor, c_visuals::icon_object_t& obj, bool is_grenade_icons = false ) {
	static std::unordered_map<const char*, bool> was_popup_open_map;
	bool& was_popup_open = was_popup_open_map[id];
	if (!was_popup_open) was_popup_open = false;
	
	c_visuals::bb_t bb = g_visuals->get_element_bb( anchor, obj.m_element_position, obj.m_element_index, obj.m_padding, (float)obj.m_size );
	ImRect im_bb = { bb.m_min, bb.m_max };
	im_bb.Expand( 2.f );
	bool right_clicked = ImGui::IsMouseClicked( ImGuiMouseButton_Right );
	bool hovering_bb = ImGui::IsMouseHoveringRect( im_bb.Min, im_bb.Max );

	if ( hovering_bb && right_clicked ) {
		ImGui::OpenPopup( id );
	}

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, hellvec2( 10, 10 ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 6.0f );

	ImGui::PushStyleColor( ImGuiCol_PopupBg, hellcolor( 20, 20, 20, 255 ).Value );
	ImGui::PushStyleColor( ImGuiCol_Border, hellcolor( 255, 255, 255, 15 ).Value );
	ImGui::SetNextWindowSize( { 230, -1 } );
	bool is_open = ImGui::IsPopupOpen( id );
	if ( ImGui::BeginPopup( id ) ) {
		hell::slider_int_passthrough( xx( "Padding" ), &obj.m_padding, 1, 20 );
		hell::slider_int_passthrough( xx( "Size" ), &obj.m_size, 8, 32 );

		ImGui::EndPopup( );
	}
	
	static std::unordered_map<const char*, int> popup_close_frame_map;
	int& popup_close_frame = popup_close_frame_map[id];
	if (!popup_close_frame_map.count(id)) popup_close_frame = -1;
	
	bool& popup_just_closed = get_popup_just_closed_flag();
	int current_frame = ImGui::GetFrameCount();
	
	if ( was_popup_open && !is_open ) {
		if (is_grenade_icons) {
			GET_VAR( int, VISUALS_PATH( m_grenade_icon_padding ) ) = obj.m_padding;
			GET_VAR( int, VISUALS_PATH( m_grenade_icon_size ) ) = obj.m_size;
		} else {
			GET_VAR( int, VISUALS_PATH( m_weapon_icon_padding ) ) = obj.m_padding;
			GET_VAR( int, VISUALS_PATH( m_weapon_icon_size ) ) = obj.m_size;
		}
		popup_just_closed = true;
		popup_close_frame = current_frame;
	}
	
	if ( popup_just_closed && current_frame > popup_close_frame ) {
		popup_just_closed = false;
	}
	
	was_popup_open = is_open;

	ImGui::PopStyleColor( 2 );
	ImGui::PopStyleVar( 3 );
}


static void handle_flags_popup(c_visuals::bb_t& anchor) {
	ImRect right_area(
		{ anchor.m_max.x, anchor.m_min.y },
		{ anchor.m_max.x + 80.f, anchor.m_min.y + 100.f }
	);

	bool use_left_icons = GET_VAR(bool, VISUALS_PATH(m_left_flags_icon_type));
	
	float offset_left = 0.f;
	auto update_side_offsets_from_element = [&](c_visuals::e_element_bb_position pos, int index, int padding, float size) {
		if (pos == c_visuals::e_element_bb_position::left) {
			auto bb = g_visuals->get_element_bb(anchor, pos, index, (float)padding, size);
			bb.normalize();
			offset_left = std::max(offset_left, anchor.m_min.x - bb.m_min.x);
		}
	};
	
	c_visuals::player_visual_data_t& data = GET_VAR(c_visuals::player_visual_data_t, VISUALS_PATH(m_visual_data_settings));
	
	update_side_offsets_from_element(data.m_health_bar.m_element_position, data.m_health_bar.m_element_index, data.m_health_bar.m_padding, data.m_health_bar.m_size);
	update_side_offsets_from_element(data.m_armor_bar.m_element_position, data.m_armor_bar.m_element_index, data.m_armor_bar.m_padding, data.m_armor_bar.m_size);
	
	if (GET_VAR(bool, VISUALS_PATH(m_name)) && data.m_name_text.m_element_position == c_visuals::e_element_bb_position::left) {
		update_side_offsets_from_element(data.m_name_text.m_element_position, data.m_name_text.m_element_index, data.m_name_text.m_padding, (float)data.m_name_text.m_size);
	}
	if (GET_VAR(bool, VISUALS_PATH(m_weapon_text)) && data.m_weapon_text.m_element_position == c_visuals::e_element_bb_position::left) {
		update_side_offsets_from_element(data.m_weapon_text.m_element_position, data.m_weapon_text.m_element_index, data.m_weapon_text.m_padding, (float)data.m_weapon_text.m_size);
	}
	if (GET_VAR(bool, VISUALS_PATH(m_weapon_icon)) && data.m_weapon_icon.m_element_position == c_visuals::e_element_bb_position::left) {
		update_side_offsets_from_element(data.m_weapon_icon.m_element_position, data.m_weapon_icon.m_element_index, data.m_weapon_icon.m_padding, (float)data.m_weapon_icon.m_size);
	}
	
	auto& flags_vec = GET_VAR(std::vector<bool>, VISUALS_PATH(m_esp_flags));
	auto flag_enabled = [&](e_esp_flags flag) -> bool {
		size_t idx = static_cast<size_t>(flag);
		return idx < flags_vec.size() && flags_vec[idx];
	};
	
	int left_flags_count = 0;
	if (flag_enabled(e_esp_flags::esp_flag_bomb)) left_flags_count++;
	if (flag_enabled(e_esp_flags::esp_flag_defuse)) left_flags_count++;
	if (flag_enabled(e_esp_flags::esp_flag_taser)) left_flags_count++;
	
	float left_area_width = 80.f;
	float left_area_height = 0.f;
	
	if (left_flags_count > 0) {
		if (use_left_icons) {
			float icon_size = (float)GET_VAR(int, VISUALS_PATH(m_left_flags_icon_size));
			float padding = (float)GET_VAR(int, VISUALS_PATH(m_esp_flags_padding));
			left_area_width = icon_size + padding + offset_left + 10.f;
			left_area_height = icon_size * left_flags_count + 2.f * (left_flags_count - 1) + 10.f;
		} else {
			float text_size = (float)GET_VAR(int, VISUALS_PATH(m_esp_flags_size));
			float padding = (float)GET_VAR(int, VISUALS_PATH(m_esp_flags_padding));
			left_area_width = 80.f + offset_left;
			left_area_height = text_size * left_flags_count + 10.f;
		}
	} else {
		left_area_height = 50.f;
	}

	ImRect left_area(
		{ anchor.m_min.x - left_area_width, anchor.m_min.y },
		{ anchor.m_min.x, anchor.m_min.y + left_area_height }
	);

	bool right_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
	bool hovering_right = ImGui::IsMouseHoveringRect(right_area.Min, right_area.Max);
	bool hovering_left = ImGui::IsMouseHoveringRect(left_area.Min, left_area.Max);

	if (hovering_right && right_clicked) {
		ImGui::SetNextWindowSize(ImVec2(230, -1), ImGuiCond_Appearing);
		ImGui::OpenPopup(xx("FLAGS_LEFT_TEXT_POPUP"));
	}

	if (hovering_left && right_clicked) {
		ImGui::SetNextWindowSize(ImVec2(230, -1), ImGuiCond_Appearing);
		if (use_left_icons) {
			ImGui::OpenPopup(xx("FLAGS_LEFT_ICONS_POPUP"));
		} else {
			ImGui::OpenPopup(xx("FLAGS_LEFT_TEXT_POPUP"));
		}
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, hellvec2(10, 10));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);

	ImGui::PushStyleColor(ImGuiCol_PopupBg, hellcolor( 20, 20, 20, 255 ).Value);
	ImGui::PushStyleColor(ImGuiCol_Border, hellcolor( 255, 255, 255, 15 ).Value);

	if (ImGui::BeginPopup(xx("FLAGS_LEFT_TEXT_POPUP"))) {
		hell::combo(xx("Font"), GET_VAR(int, VISUALS_PATH(m_esp_flags_font_type)), font_names, IM_ARRAYSIZE(font_names));
		hell::combo(xx("Shadow"), GET_VAR(int, VISUALS_PATH(m_esp_flags_shadow_type)), shadow_names, IM_ARRAYSIZE(shadow_names));
		hell::slider_int_passthrough(xx("Size"), &GET_VAR(int, VISUALS_PATH(m_esp_flags_size)), 1, 16);
		hell::slider_int_passthrough(xx("Padding"), &GET_VAR(int, VISUALS_PATH(m_esp_flags_padding)), 0, 32);
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup(xx("FLAGS_LEFT_ICONS_POPUP"))) {
		hell::slider_int_passthrough(xx("Padding"), &GET_VAR(int, VISUALS_PATH(m_esp_flags_padding)), 0, 32);
		hell::slider_int_passthrough(xx("Size"), &GET_VAR(int, VISUALS_PATH(m_left_flags_icon_size)), 8, 32);
		ImGui::EndPopup();
	}

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
}

bool visual_preview::esp_preview( ImDrawList* draw_list, c_visuals::bb_t& bbox ) {
	//m_model_preview.init( );

	//m_model_preview.draw_model( );

	c_visuals::player_visual_data_t& data = GET_VAR( c_visuals::player_visual_data_t, VISUALS_PATH( m_visual_data_settings ) );

	static uint64 dragging_id = -1;

	data.m_preview_mode = true;
	data.m_draw_list = draw_list;
	
	data.m_name_text.m_element_position = static_cast<c_visuals::e_element_bb_position>(GET_VAR(int, VISUALS_PATH(m_name_position)));
	data.m_name_text.m_element_index = GET_VAR(int, VISUALS_PATH(m_name_index));
	data.m_name_text.m_text_offset_x = 0.0f;
	
	data.m_weapon_text.m_element_position = static_cast<c_visuals::e_element_bb_position>(GET_VAR(int, VISUALS_PATH(m_weapon_text_position)));
	data.m_weapon_text.m_element_index = GET_VAR(int, VISUALS_PATH(m_weapon_text_index));
	data.m_weapon_text.m_text_offset_x = 0.0f;
	
	data.m_armor_bar.m_element_position = static_cast<c_visuals::e_element_bb_position>(GET_VAR(int, VISUALS_PATH(m_armor_bar_position)));
	data.m_armor_bar.m_element_index = GET_VAR(int, VISUALS_PATH(m_armor_bar_index));
	
	if (data.m_weapon_text.m_element_position == c_visuals::e_element_bb_position::bottom) {
		if (GET_VAR(bool, VISUALS_PATH(m_armor_bar)) && data.m_armor_bar.m_element_position == c_visuals::e_element_bb_position::bottom) {
			if (data.m_weapon_text.m_element_index <= data.m_armor_bar.m_element_index) {
				data.m_weapon_text.m_element_index = data.m_armor_bar.m_element_index + 1;
			}
		}
	}

	{ // bb
		data.m_bb = bbox;
		data.m_bb.m_bb_bg_color = GET_VAR( hellcolor, VISUALS_PATH( m_bb_bg_color ) );
		data.m_bb.m_bb_fg_color = GET_VAR( hellcolor, VISUALS_PATH( m_bb_fg_color ) );
	}

	if ( GET_VAR( bool, VISUALS_PATH( m_health_bar ) ) ) {
		data.m_health_bar.m_value = 50.f;
		data.m_health_bar.m_fg_color = GET_VAR( hellcolor, VISUALS_PATH( m_health_bar_color ) );
		data.m_health_bar.m_bg_color = GET_VAR( hellcolor, VISUALS_PATH( m_health_bar_color_bg ) );
	
		data.m_health_bar.m_glow_intensity = 1.2f;
		data.m_health_bar.m_gradient_enabled = GET_VAR( bool, VISUALS_PATH( m_health_bar_gradient_enabled ) );
		data.m_health_bar.m_gradient_color = GET_VAR( hellcolor, VISUALS_PATH( m_health_bar_gradient_color ) );
		data.m_health_bar.m_gradient_reverse = false;
	}

	if ( GET_VAR( bool, VISUALS_PATH( m_armor_bar ) ) ) {
		data.m_armor_bar.m_value = 50.f;
		data.m_armor_bar.m_fg_color = GET_VAR( hellcolor, VISUALS_PATH( m_armor_bar_color ) );
		data.m_armor_bar.m_bg_color = GET_VAR( hellcolor, VISUALS_PATH( m_armor_bar_color_bg ) );
		
	
		data.m_armor_bar.m_gradient_enabled = GET_VAR( bool, VISUALS_PATH( m_armor_bar_gradient_enabled ) );
		data.m_armor_bar.m_gradient_color = GET_VAR( hellcolor, VISUALS_PATH( m_armor_bar_gradient_color ) );
		data.m_armor_bar.m_gradient_reverse = true;
	}

	handle_flags_popup(data.m_bb);

	if ( GET_VAR( bool, VISUALS_PATH( m_name ) ) ) {
		data.m_name_text.m_text = xx( "quint" );
		data.m_name_text.m_fg_color = GET_VAR( hellcolor, VISUALS_PATH( m_name_color ) );
		data.m_name_text.m_bg_color = GET_VAR( hellcolor, VISUALS_PATH( m_name_color_bg ) );

		{
			handle_text_popup( data.m_bb, data.m_name_text );

			auto handle_preview_bar_result = handle_preview_object( draw_list, data.m_bb, e_preview_type::preview_type_text, data.m_name_text );
			if ( handle_preview_bar_result.first && !m_is_dragging_esp_element ) {
				m_is_dragging_esp_element = true;
				dragging_id = fnv_hash( "NAME_TEXT" );
			}
		}
	}

	if ( GET_VAR( bool, VISUALS_PATH( m_weapon_text ) ) ) {
		data.m_weapon_text.m_text = xx( "SSG 08" );
		data.m_weapon_text.m_fg_color = GET_VAR( hellcolor, VISUALS_PATH( m_weapon_name_color ) );
		data.m_weapon_text.m_bg_color = GET_VAR( hellcolor, VISUALS_PATH( m_weapon_name_color_bg ) );

		{
			handle_text_popup( data.m_bb, data.m_weapon_text );

			auto handle_preview_bar_result = handle_preview_object( draw_list, data.m_bb, e_preview_type::preview_type_text, data.m_weapon_text );
			if ( handle_preview_bar_result.first && !m_is_dragging_esp_element ) {
				m_is_dragging_esp_element = true;
				dragging_id = fnv_hash( "WEAPON_TEXT" );
			}
		}
	}

	if ( GET_VAR( bool, VISUALS_PATH( m_weapon_icon ) ) ) {
		handle_icon_popup( xx( "WEAPON_ICON_POPUP" ), data.m_bb, data.m_weapon_icon );
		
		bool is_popup_open = ImGui::IsPopupOpen( xx( "WEAPON_ICON_POPUP" ) );
		bool popup_just_closed = get_popup_just_closed_flag();
		
		if ( dragging_id != fnv_hash( "WEAPON_ICON" ) && !is_popup_open && !popup_just_closed ) {
			data.m_weapon_icon.m_color = GET_VAR( hellcolor, VISUALS_PATH( m_weapon_icon_color ) );
			data.m_weapon_icon.m_size = GET_VAR( int, VISUALS_PATH( m_weapon_icon_size ) );
			data.m_weapon_icon.m_padding = GET_VAR( int, VISUALS_PATH( m_weapon_icon_padding ) );
			data.m_weapon_icon.m_element_position = static_cast<c_visuals::e_element_bb_position>(GET_VAR(int, VISUALS_PATH(m_weapon_icon_position)));
			data.m_weapon_icon.m_element_index = GET_VAR(int, VISUALS_PATH(m_weapon_icon_index));
			
			if (data.m_weapon_icon.m_element_position == c_visuals::e_element_bb_position::bottom) {
				if (GET_VAR(bool, VISUALS_PATH(m_weapon_text)) && data.m_weapon_text.m_element_position == c_visuals::e_element_bb_position::bottom) {
					if (data.m_weapon_icon.m_element_index <= data.m_weapon_text.m_element_index) {
						data.m_weapon_icon.m_element_index = data.m_weapon_text.m_element_index + 1;
					}
				}
			}
		}

		icon_data_t icon_data = get_weapon_icon_preview(WEAPON_SSG_08);
		if (icon_data.texture_view && icon_data.width > 0 && icon_data.height > 0)
		{
			g_visuals->draw_icon( data.m_bb, data.m_weapon_icon, icon_data, 1.f, draw_list );
		}

		{
			auto handle_preview_icon_result = handle_preview_object( draw_list, data.m_bb, e_preview_type::preview_type_icon, data.m_weapon_icon );
			if ( handle_preview_icon_result.first && !m_is_dragging_esp_element ) {
				m_is_dragging_esp_element = true;
				dragging_id = fnv_hash( "WEAPON_ICON" );
			}
		}
	}

	if (GET_VAR(bool, VISUALS_PATH(m_grenade_icons))) {
		handle_icon_popup(xx("GRENADE_ICONS_POPUP"), data.m_bb, data.m_grenade_icons, true);
		
		bool is_popup_open = ImGui::IsPopupOpen(xx("GRENADE_ICONS_POPUP"));
		bool popup_just_closed = get_popup_just_closed_flag();
		
		if (dragging_id != fnv_hash("GRENADE_ICONS") && !is_popup_open && !popup_just_closed) {
			data.m_grenade_icons.m_color = GET_VAR(hellcolor, VISUALS_PATH(m_grenade_icons_color));
			data.m_grenade_icons.m_size = GET_VAR(int, VISUALS_PATH(m_grenade_icon_size));
			data.m_grenade_icons.m_padding = GET_VAR(int, VISUALS_PATH(m_grenade_icon_padding));
			data.m_grenade_icons.m_element_position = static_cast<c_visuals::e_element_bb_position>(GET_VAR(int, VISUALS_PATH(m_grenade_icon_position)));
			data.m_grenade_icons.m_element_index = GET_VAR(int, VISUALS_PATH(m_grenade_icon_index));
		}

		std::vector<icon_data_t> grenade_icons_preview;
		grenade_icons_preview.push_back(get_panorama_texture("icons/equipment/hegrenade"));
		grenade_icons_preview.push_back(get_panorama_texture("icons/equipment/flashbang"));
		grenade_icons_preview.push_back(get_panorama_texture("icons/equipment/molotov"));
		grenade_icons_preview.push_back(get_panorama_texture("icons/equipment/smokegrenade"));
		grenade_icons_preview.push_back(get_panorama_texture("icons/equipment/decoy"));

		g_visuals->draw_grenade_icons(
			data.m_bb,
			data.m_grenade_icons,
			grenade_icons_preview,
			1.f,
			draw_list
		);

		auto handle_preview_icon_result = handle_preview_object(draw_list, data.m_bb, e_preview_type::preview_type_icon, data.m_grenade_icons);
		if (handle_preview_icon_result.first && !m_is_dragging_esp_element) {
			m_is_dragging_esp_element = true;
			dragging_id = fnv_hash("GRENADE_ICONS");
		}
	}

	if ( GET_VAR( bool, VISUALS_PATH( m_health_bar ) ) ) {
			handle_health_bar_popup( data.m_bb, data.m_health_bar );

			auto handle_preview_bar_result = handle_preview_object( draw_list, data.m_bb, e_preview_type::preview_type_bar, data.m_health_bar );
			if ( handle_preview_bar_result.first && !m_is_dragging_esp_element ) {
				m_is_dragging_esp_element = true;
				dragging_id = fnv_hash( "HEALTH_BAR" );
			}
	}

	if ( GET_VAR( bool, VISUALS_PATH( m_armor_bar ) ) ) {
		{
			handle_armor_bar_popup( data.m_bb, data.m_armor_bar );

			auto handle_preview_bar_result = handle_preview_object( draw_list, data.m_bb, e_preview_type::preview_type_bar, data.m_armor_bar );
			if ( handle_preview_bar_result.first && !m_is_dragging_esp_element ) {
				m_is_dragging_esp_element = true;
				dragging_id = fnv_hash( "ARMOR_BAR" );
			}
		}
	}

	data.m_alpha_modifier = g_animation->handle_anim( ImGui::GetID( xx( "ALPHA_MODIFIER" ) ), m_is_dragging_esp_element, 1.f, 0.5f, 8.f ).get_current_val( );

	if ( m_is_dragging_esp_element ) {
		ImGui::SetNextFrameWantCaptureMouse(false);
		ImGui::GetIO().WantCaptureMouse = false;
		
		ImGuiContext& g = *ImGui::GetCurrentContext();
		if (g.MovingWindow != nullptr) {
			g.MovingWindow = nullptr;
			ImGui::ClearActiveID();
		}
		
		switch ( dragging_id ) {
		case fnv_hash( "ARMOR_BAR" ):
			handle_drag(
				data.m_bb,
				( int& )data.m_armor_bar.m_element_position,
				data.m_armor_bar.m_element_index,
				data.m_armor_bar.m_padding,
				data.m_armor_bar.m_size,
				data.m_draw_list
			);
			GET_VAR(int, VISUALS_PATH(m_armor_bar_position)) = (int)data.m_armor_bar.m_element_position;
			GET_VAR(int, VISUALS_PATH(m_armor_bar_index)) = data.m_armor_bar.m_element_index;
			break;
		case fnv_hash( "HEALTH_BAR" ):
			handle_drag(
				data.m_bb,
				( int& )data.m_health_bar.m_element_position,
				data.m_health_bar.m_element_index,
				data.m_health_bar.m_padding,
				data.m_health_bar.m_size,
				data.m_draw_list
			);
			break;
		case fnv_hash( "NAME_TEXT" ):
			handle_drag(
				data.m_bb,
				( int& )data.m_name_text.m_element_position,
				data.m_name_text.m_element_index,
				data.m_name_text.m_padding,
				data.m_name_text.m_size,
				data.m_draw_list
			);
			GET_VAR(int, VISUALS_PATH(m_name_position)) = (int)data.m_name_text.m_element_position;
			GET_VAR(int, VISUALS_PATH(m_name_index)) = data.m_name_text.m_element_index;
			break;
		case fnv_hash( "WEAPON_TEXT" ):
			handle_drag(
				data.m_bb,
				( int& )data.m_weapon_text.m_element_position,
				data.m_weapon_text.m_element_index,
				data.m_weapon_text.m_padding,
				data.m_weapon_text.m_size,
				data.m_draw_list
			);
			GET_VAR(int, VISUALS_PATH(m_weapon_text_position)) = (int)data.m_weapon_text.m_element_position;
			GET_VAR(int, VISUALS_PATH(m_weapon_text_index)) = data.m_weapon_text.m_element_index;
			break;
		case fnv_hash( "WEAPON_ICON" ):
			handle_drag(
				data.m_bb,
				( int& )data.m_weapon_icon.m_element_position,
				data.m_weapon_icon.m_element_index,
				data.m_weapon_icon.m_padding,
				data.m_weapon_icon.m_size,
				data.m_draw_list
			);
			GET_VAR(int, VISUALS_PATH(m_weapon_icon_position)) = (int)data.m_weapon_icon.m_element_position;
			GET_VAR(int, VISUALS_PATH(m_weapon_icon_index)) = data.m_weapon_icon.m_element_index;
			break;
		case fnv_hash("GRENADE_ICONS"):
			handle_drag(
				data.m_bb,
				(int&)data.m_grenade_icons.m_element_position,
				data.m_grenade_icons.m_element_index,
				data.m_grenade_icons.m_padding,
				data.m_grenade_icons.m_size,
				data.m_draw_list
			);
			break;
		}
	}

	if ( m_is_dragging_esp_element && !ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
		ImGui::SetNextFrameWantCaptureMouse(true);
		ImGui::GetIO().WantCaptureMouse = true;
		
		if (dragging_id == fnv_hash("GRENADE_ICONS")) {
			GET_VAR(int, VISUALS_PATH(m_grenade_icon_position)) = (int)data.m_grenade_icons.m_element_position;
			GET_VAR(int, VISUALS_PATH(m_grenade_icon_index)) = data.m_grenade_icons.m_element_index;
		}
		m_is_dragging_esp_element = false;
		dragging_id = -1;
	} else if ( m_is_dragging_esp_element && dragging_id == -1 ) {
		ImGui::SetNextFrameWantCaptureMouse(true);
		ImGui::GetIO().WantCaptureMouse = true;
		m_is_dragging_esp_element = false;
	}

	if (m_is_dragging_esp_element) {
		ImGui::SetNextFrameWantCaptureMouse(false);
		ImGui::GetIO().WantCaptureMouse = false;
	}

	g_visuals->draw_player_visual_data( data );

	return m_is_dragging_esp_element;
}
