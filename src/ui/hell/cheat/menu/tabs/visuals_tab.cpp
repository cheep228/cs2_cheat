#include "visuals_tab.h"

#include <includes.h>
#include <cheat/config/config_system.h>
#include <cheat/config/vars.h>
#include <cheat/input.h>

#include <cheat/menu/hell_gui/hell_gui.h>
#include <cheat/menu/hell_gui/colors.h>
#include <utils/fonts/font_manager.h>
#include <cheat/features/visuals/visuals.h>
#include <cheat/menu/animation/anim.h>
#include <cheat/menu/visual_preview.h>
#include <cheat/features/visuals/skybox_changer.h>

using namespace tabs::visuals;

const char* chams_type_bt_names[] = {
	xx("Last"),
	xx("All"),
};

const char* chams_material[] = {
	xx("Default"),
	xx("Latex"),
	xx("Glow"),
	xx("Ghost"),
	xx("Flat")
};

const char* weather_particles[] = {
	xx("Rain"),
	xx("Ash")
};

const char* removal_names[] = {
	xx("Scope"),
	xx("Local overhead"),
	xx("Team overhead"),
	xx("Smoke"),
	xx("Flashbang overlay"),
	xx("Torso"),
	xx("Visual recoil")
};

const char* scope_overlay_names[] = {
	xx("Gradient"),
	xx("Overlay")
};

const char* esp_flag_names[] = {
	xx("Scoped"),
	xx("Bomb"),
	xx("Taser"),
	xx("Kits"),
	xx("Defusing"),
	xx("Armor"),
	xx("Helmet"),
	xx("Reloading"),
	xx("Ping")
};

const char* kill_effect_names[] = {
		xx("Firework 1"),
		xx("Firework 2"),
		xx("Feathers"),
		xx("Explosion"),
		xx("Glow Stars")
};

#define KILL_EFFECT_NAMES_COUNT 5

void tabs::render_visuals() {

	static int current_subtab = e_subtab::subtab_local;

	{
		static std::array<const char*, e_subtab::subtab_max> button_names = {
			xx("Local"),      // 0
			xx("Enemy"),      // 1
			xx("World"),      // 2
			xx("Other")       // 3
		};

		static hell::button_data_t selected = { { 255, 255, 255 }, { 255, 255, 255 }, { 255, 255, 255 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
		static hell::button_data_t unselected = { { 150, 150, 150 }, { 180, 180, 180 }, { 180, 180, 180 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };


		std::pair< bool, int > subtab_result = hell::button_array< button_names.size() >(
			ImGui::GetID(xx("VISUALS_SUBTAB")),
			button_names,
			true,
			selected,
			unselected,
			true,
			hellcolor(140, 90, 190, 255)
		);
		if (subtab_result.first) {
			current_subtab = (e_subtab)subtab_result.second;
		}
	}

	hell::set_cursor_pos_relative_y();

	hellvec2 child_size = hell::calculate_column_size(2);

	switch (current_subtab) {
	case e_subtab::subtab_local: {
		float spacing = 18.f;
		float start_y = ImGui::GetCursorPosY();
		float col_spacing = ImGui::GetStyle().WindowPadding.x;

		float col1_x = ImGui::GetCursorPosX();
		float col2_x = col1_x + child_size.x + col_spacing;

		float model_h = hell::calculate_child_height([] {
		hell::color_picker(xx("Local visible chams color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_local_color)), 2, true);
			hell::color_picker(xx("Local occluded chams color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_local_color_invis)), 3, true);
		hell::checkbox(xx("Chams"), VISUALS_PATH(m_chams_local_enabled), [] {
			hell::combo(xx("Visible material"), GET_VAR(int, VISUALS_PATH(m_chams_local_material)), chams_material, e_materials::material_max);
			hell::combo(xx("Occluded material"), GET_VAR(int, VISUALS_PATH(m_chams_local_material_invis)), chams_material, e_materials::material_max);
				if (GET_VAR(int, VISUALS_PATH(m_chams_local_material)) != e_materials::material_glow) {
				hell::checkbox(xx("Visible overlay"), VISUALS_PATH(m_glow_chams_local_visible));
				}
				if (GET_VAR(int, VISUALS_PATH(m_chams_local_material_invis)) != e_materials::material_glow) {
					hell::checkbox(xx("Occluded overlay"), VISUALS_PATH(m_glow_chams_local_invisible));
			}
			}, 1);
		hell::color_picker(xx("Local attachments chams color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_local_attachments_color)), 2, true);
		hell::checkbox(xx("Attachments"), VISUALS_PATH(m_chams_local_attachments_enabled), [] {
				hell::combo(xx("Material"), GET_VAR(int, VISUALS_PATH(m_chams_local_attachments_material)), chams_material, e_materials::material_max);
				}, 1);
		hell::color_picker(xx("Transparency color"), GET_VAR(hellcolor, VISUALS_PATH(m_transparency_color)), 1, true);
		hell::checkbox(xx("Transparency in scope"), VISUALS_PATH(m_transparency_in_scope));
			hell::color_picker(xx("Local glow color"), GET_VAR(hellcolor, VISUALS_PATH(m_local_glow_color)), 1, true);
			hell::checkbox(xx("Glow"), VISUALS_PATH(m_local_glow));

		hell::color_picker(xx("Viewmodel arms visible color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_viewmodel_arms_color)), 2, true);
		hell::checkbox(xx("Viewmodel arms"), VISUALS_PATH(m_chams_viewmodel_arms_enabled), [] {
				hell::combo(xx("Visible material"), GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_arms_material)), chams_material, e_materials::material_max);
			hell::combo(xx("Occluded material"), GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_arms_material_invis)), chams_material, e_materials::material_max);
			if (GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_arms_material)) != e_materials::material_glow)
				hell::checkbox(xx("Visible overlay"), VISUALS_PATH(m_glow_chams_viewmodel_arms_visible));
			if (GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_arms_material_invis)) != e_materials::material_glow)
				hell::checkbox(xx("Occluded overlay"), VISUALS_PATH(m_glow_chams_viewmodel_arms_invisible));
				}, 2);

		hell::color_picker(xx("Viewmodel weapon visible color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_viewmodel_weapon_color)), 2, true);
			hell::checkbox(xx("Viewmodel weapon"), VISUALS_PATH(m_chams_viewmodel_weapon_enabled), [] {
				hell::combo(xx("Visible material"), GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_weapon_material)), chams_material, e_materials::material_max);
			hell::combo(xx("Occluded material"), GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_weapon_material_invis)), chams_material, e_materials::material_max);
			if (GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_weapon_material)) != e_materials::material_glow)
					hell::checkbox(xx("Visible overlay"), VISUALS_PATH(m_glow_chams_viewmodel_weapon_visible));
				if (GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_weapon_material_invis)) != e_materials::material_glow)
					hell::checkbox(xx("Occluded overlay"), VISUALS_PATH(m_glow_chams_viewmodel_weapon_invisible));
			}, 3);
			});

		float overlay_h = hell::calculate_child_height([] {
			if (GET_VAR(int, VISUALS_PATH(m_scope_type)) == e_scope_type::scope_type_static) {
				hell::color_picker(xx("Scope color inside"), GET_VAR(hellcolor, VISUALS_PATH(m_scope_col_inside)), 2, true, false);
				hell::color_picker(xx("Scope color outside"), GET_VAR(hellcolor, VISUALS_PATH(m_scope_col_outside)), 3, true, false);
			}
			hell::checkbox(xx("Scope overlay"), VISUALS_PATH(m_scope_overlay), [] {
				hell::combo(xx("Type"), GET_VAR(int, VISUALS_PATH(m_scope_type)), scope_overlay_names, IM_ARRAYSIZE(scope_overlay_names));
				if (GET_VAR(int, VISUALS_PATH(m_scope_type)) == e_scope_type::scope_type_static) {
					hell::slider_int(xx("Gap"), VISUALS_PATH(m_scope_overlay_gap), 0, 100, 0, false, xx("%dpx"));
					hell::slider_int(xx("Length"), VISUALS_PATH(m_scope_overlay_length), 10, 500, 0, false, xx("%dpx"));
					hell::checkbox(xx("Dynamic gap"), VISUALS_PATH(m_enabled_spread_gap));
				}
				});
			hell::color_picker(xx("Spread circle inner color"), GET_VAR(hellcolor, VISUALS_PATH(m_enabled_spread_circle_color_first)), 1, true);
			hell::color_picker(xx("Spread circle outer color"), GET_VAR(hellcolor, VISUALS_PATH(m_enabled_spread_circle_color_last)), 2, true);
			hell::checkbox(xx("Spread overlay"), VISUALS_PATH(m_enabled_spread_circle));
			});

		float world_h = hell::calculate_child_height([] {
			hell::checkbox(xx("Thirdperson"), VISUALS_PATH(m_third_person_enabled),
				[] {
					hell::slider_int(xx("Distance"), VISUALS_PATH(m_third_person_distance), 20, 160, 0, false, xx("%du"));
				}
			);
			hell::checkbox(xx("Override FOV"), VISUALS_PATH(m_override_world_fov),
				[] {
					hell::slider_int(xx("World"), VISUALS_PATH(m_override_world_fov_value), 60, 140, 0, false, xx("%ddeg"));
					hell::slider_none(xx("First scope"), VISUALS_PATH(m_override_world_fov_value_first_scope), 30, 140, 0, xx("%ddeg"));
					hell::slider_none(xx("Second scope"), VISUALS_PATH(m_override_world_fov_value_second_scope), 30, 140, 0, xx("%ddeg"));
					hell::checkbox(xx("Dynamic FOV"), VISUALS_PATH(m_dynamic_fov));
				}
			);
			
			hell::checkbox(xx("Viewmodel changer"), VISUALS_PATH(m_override_viewmodel_fov),
				[] {
					hell::slider_int(xx("Viewmodel FOV"), VISUALS_PATH(m_override_viewmodel_value), 40, 120, 0, false, xx("%ddeg"));
					hell::slider_int(xx("Viewmodel X"), VISUALS_PATH(m_override_viewmodel_value_fov_x), -90, 90, 0, false, xx("%ddeg"));
					hell::slider_int(xx("Viewmodel Y"), VISUALS_PATH(m_override_viewmodel_value_fov_y), -90, 90, 0, false, xx("%ddeg"));
					hell::slider_int(xx("Viewmodel Z"), VISUALS_PATH(m_override_viewmodel_value_fov_z), -90, 90, 0, false, xx("%ddeg"));
				}
			);
			hell::color_picker(xx("Impact box color"), GET_VAR(hellcolor, VISUALS_PATH(m_local_impact_boxes_color)), 2, true);
			hell::checkbox(xx("Impact box"), VISUALS_PATH(m_local_impact_boxes), [] {
				hell::slider_int(xx("Duration"), VISUALS_PATH(m_local_impact_boxes_duration), 1, 15, 0, false, xx("%ds"));
				hell::slider_int(xx("Size"), VISUALS_PATH(m_local_impact_boxes_size), 1, 5, 0, false, xx("%du"));
				});
		
			});

		ImGui::SetCursorPos({ col1_x, start_y });
		hell::child(xx("Model"), { child_size.x, model_h }, [] {
		hell::color_picker(xx("Local visible chams color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_local_color)), 2, true);
			hell::color_picker(xx("Local occluded chams color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_local_color_invis)), 3, true);
		hell::checkbox(xx("Chams"), VISUALS_PATH(m_chams_local_enabled), [] {
			hell::combo(xx("Visible material"), GET_VAR(int, VISUALS_PATH(m_chams_local_material)), chams_material, e_materials::material_max);
				hell::combo(xx("Occluded material"), GET_VAR(int, VISUALS_PATH(m_chams_local_material_invis)), chams_material, e_materials::material_max);
				if (GET_VAR(int, VISUALS_PATH(m_chams_local_material)) != e_materials::material_glow) {
					hell::checkbox(xx("Visible overlay"), VISUALS_PATH(m_glow_chams_local_visible));
				}
			if (GET_VAR(int, VISUALS_PATH(m_chams_local_material_invis)) != e_materials::material_glow) {
					hell::checkbox(xx("Occluded overlay"), VISUALS_PATH(m_glow_chams_local_invisible));
				}
			}, 1);

			hell::color_picker(xx("Local attachments chams color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_local_attachments_color)), 2, true);
			hell::checkbox(xx("Attachments"), VISUALS_PATH(m_chams_local_attachments_enabled), [] {
				hell::combo(xx("Material"), GET_VAR(int, VISUALS_PATH(m_chams_local_attachments_material)), chams_material, e_materials::material_max);
				}, 1);

			hell::color_picker(xx("Transparency color"), GET_VAR(hellcolor, VISUALS_PATH(m_transparency_color)), 1, true);
			hell::checkbox(xx("Transparency in scope"), VISUALS_PATH(m_transparency_in_scope));
			hell::color_picker(xx("Local glow color"), GET_VAR(hellcolor, VISUALS_PATH(m_local_glow_color)), 1, true);
			hell::checkbox(xx("Glow"), VISUALS_PATH(m_local_glow));

			hell::color_picker(xx("Viewmodel arms visible color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_viewmodel_arms_color)), 2, true);
			hell::checkbox(xx("Viewmodel arms"), VISUALS_PATH(m_chams_viewmodel_arms_enabled), [] {
				hell::combo(xx("Visible material"), GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_arms_material)), chams_material, e_materials::material_max);
				hell::combo(xx("Occluded material"), GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_arms_material_invis)), chams_material, e_materials::material_max);
			if (GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_arms_material)) != e_materials::material_glow)
					hell::checkbox(xx("Visible overlay"), VISUALS_PATH(m_glow_chams_viewmodel_arms_visible));
				if (GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_arms_material_invis)) != e_materials::material_glow)
					hell::checkbox(xx("Occluded overlay"), VISUALS_PATH(m_glow_chams_viewmodel_arms_invisible));
				}, 2);

			hell::color_picker(xx("Viewmodel weapon visible color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_viewmodel_weapon_color)), 2, true);
			hell::checkbox(xx("Viewmodel weapon"), VISUALS_PATH(m_chams_viewmodel_weapon_enabled), [] {
				hell::combo(xx("Visible material"), GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_weapon_material)), chams_material, e_materials::material_max);
				hell::combo(xx("Occluded material"), GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_weapon_material_invis)), chams_material, e_materials::material_max);
				if (GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_weapon_material)) != e_materials::material_glow)
					hell::checkbox(xx("Visible overlay"), VISUALS_PATH(m_glow_chams_viewmodel_weapon_visible));
				if (GET_VAR(int, VISUALS_PATH(m_chams_viewmodel_weapon_material_invis)) != e_materials::material_glow)
					hell::checkbox(xx("Occluded overlay"), VISUALS_PATH(m_glow_chams_viewmodel_weapon_invisible));
			}, 3);
			}
		);

		ImGui::SetCursorPos({ col2_x, start_y });
		hell::child(xx("Overlay"), { child_size.x, overlay_h }, [] {
			if (GET_VAR(int, VISUALS_PATH(m_scope_type)) == e_scope_type::scope_type_static) {
				hell::color_picker(xx("Scope color inside"), GET_VAR(hellcolor, VISUALS_PATH(m_scope_col_inside)), 2, true, false);
				hell::color_picker(xx("Scope color outside"), GET_VAR(hellcolor, VISUALS_PATH(m_scope_col_outside)), 3, true, false);
			}
			hell::checkbox(xx("Scope overlay"), VISUALS_PATH(m_scope_overlay), [] {
				hell::combo(xx("Type"), GET_VAR(int, VISUALS_PATH(m_scope_type)), scope_overlay_names, IM_ARRAYSIZE(scope_overlay_names));
				if (GET_VAR(int, VISUALS_PATH(m_scope_type)) == e_scope_type::scope_type_static) {
					hell::slider_int(xx("Gap"), VISUALS_PATH(m_scope_overlay_gap), 0, 100, 0, false, xx("%dpx"));
					hell::slider_int(xx("Length"), VISUALS_PATH(m_scope_overlay_length), 10, 500, 0, false, xx("%dpx"));
					hell::checkbox(xx("Dynamic gap"), VISUALS_PATH(m_enabled_spread_gap));
				}
				});

			hell::color_picker(xx("Spread circle inner color"), GET_VAR(hellcolor, VISUALS_PATH(m_enabled_spread_circle_color_first)), 1, true);
			hell::color_picker(xx("Spread circle outer color"), GET_VAR(hellcolor, VISUALS_PATH(m_enabled_spread_circle_color_last)), 2, true);
			hell::checkbox(xx("Visualize spread"), VISUALS_PATH(m_enabled_spread_circle));
			}
		);

		bool world_under_first = model_h <= overlay_h;
		ImGui::SetCursorPos({ world_under_first ? col1_x : col2_x, start_y + (world_under_first ? model_h : overlay_h) + spacing });
		hell::child(xx("World"), { child_size.x, world_h }, [] {
			hell::checkbox(xx("Thirdperson"), VISUALS_PATH(m_third_person_enabled),
				[] {
					hell::slider_int(xx("Distance"), VISUALS_PATH(m_third_person_distance), 20, 160, 0, false, xx("%du"));
				}
			);

			hell::checkbox(xx("Override FOV"), VISUALS_PATH(m_override_world_fov),
				[] {
					hell::slider_int(xx("World"), VISUALS_PATH(m_override_world_fov_value), 60, 140, 0, false, xx("%ddeg"));
					hell::slider_none(xx("First scope"), VISUALS_PATH(m_override_world_fov_value_first_scope), 30, 140, 0, xx("%ddeg"));
					hell::slider_none(xx("Second scope"), VISUALS_PATH(m_override_world_fov_value_second_scope), 30, 140, 0, xx("%ddeg"));
					hell::checkbox(xx("Dynamic FOV"), VISUALS_PATH(m_dynamic_fov));

				}
			);

			hell::checkbox(xx("Viewmodel changer"), VISUALS_PATH(m_override_viewmodel_fov),
				[] {
					hell::slider_int(xx("Viewmodel FOV"), VISUALS_PATH(m_override_viewmodel_value), 40, 120, 0, false, xx("%ddeg"));
					hell::slider_int(xx("Viewmodel X"), VISUALS_PATH(m_override_viewmodel_value_fov_x), -90, 90, 0, false, xx("%ddeg"));
					hell::slider_int(xx("Viewmodel Y"), VISUALS_PATH(m_override_viewmodel_value_fov_y), -90, 90, 0, false, xx("%ddeg"));
					hell::slider_int(xx("Viewmodel Z"), VISUALS_PATH(m_override_viewmodel_value_fov_z), -90, 90, 0, false, xx("%ddeg"));

				}
			);

			hell::color_picker(xx("Impact box color"), GET_VAR(hellcolor, VISUALS_PATH(m_local_impact_boxes_color)), 2, true);
			hell::checkbox(xx("Impact box"), VISUALS_PATH(m_local_impact_boxes), [] {
				hell::slider_int(xx("Duration"), VISUALS_PATH(m_local_impact_boxes_duration), 1, 15, 0, false, xx("%ds"));
				hell::slider_int(xx("Size"), VISUALS_PATH(m_local_impact_boxes_size), 1, 5, 0, false, xx("%du"));
				});

		
			}
		);

		break;
	}
	case e_subtab::subtab_enemy: {
		float spacing = 18.f;
		float start_y = ImGui::GetCursorPosY();
		float col_spacing = ImGui::GetStyle().WindowPadding.x;

		float col1_x = ImGui::GetCursorPosX();
		float col2_x = col1_x + child_size.x + col_spacing;

		float esp_h = hell::calculate_child_height([] {
			hell::checkbox(xx("Enable"), VISUALS_PATH(m_esp_enabled));
			hell::color_picker(xx("Bounding box color"), GET_VAR(hellcolor, VISUALS_PATH(m_bb_fg_color)), 1, true);
			hell::color_picker(xx("Bounding box outline color"), GET_VAR(hellcolor, VISUALS_PATH(m_bb_bg_color)), 2, true);
			hell::checkbox(xx("Bounding box"), VISUALS_PATH(m_bbox));
			hell::color_picker(xx("Name Color"), GET_VAR(hellcolor, VISUALS_PATH(m_name_color)), 1, true);
			hell::color_picker(xx("Name Color Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_name_color_bg)), 2, true);
			hell::checkbox(xx("Name"), VISUALS_PATH(m_name));
			hell::checkbox(xx("Steam avatar"), VISUALS_PATH(m_enabled_steam_avatar));
			hell::color_picker(xx("Health Bar Color"), GET_VAR(hellcolor, VISUALS_PATH(m_health_bar_color)), 1, true);
			hell::color_picker(xx("Health Bar Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_health_bar_color_bg)), 2, true);
			hell::checkbox(xx("Health bar"), VISUALS_PATH(m_health_bar));
			hell::color_picker(xx("Armor Bar Color"), GET_VAR(hellcolor, VISUALS_PATH(m_armor_bar_color)), 1, true);
			hell::color_picker(xx("Armor Bar Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_armor_bar_color_bg)), 2, true);
			hell::checkbox(xx("Armor bar"), VISUALS_PATH(m_armor_bar));
			hell::color_picker(xx("Item Text Color"), GET_VAR(hellcolor, VISUALS_PATH(m_weapon_name_color)), 1, true);
			hell::color_picker(xx("Item Text Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_weapon_name_color_bg)), 2, true);
			hell::checkbox(xx("Item text"), VISUALS_PATH(m_weapon_text));
			hell::color_picker(xx("Item Icon Color"), GET_VAR(hellcolor, VISUALS_PATH(m_weapon_icon_color)), 1, true);
			hell::checkbox(xx("Item icon"), VISUALS_PATH(m_weapon_icon));
			hell::color_picker(xx("Flags Color"), GET_VAR(hellcolor, VISUALS_PATH(m_esp_flags_color)), 8, true);
			hell::multi_combo(xx("Flags"), esp_flag_names, GET_VAR(std::vector<bool>, VISUALS_PATH(m_esp_flags)), e_esp_flags::esp_flag_max);
			hell::color_picker(xx("Skeleton ESP Color"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_color)), 1, true);
			hell::color_picker(xx("Skeleton ESP Outline Color"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_outline_color)), 2, true);
			hell::checkbox(xx("Skeleton ESP"), VISUALS_PATH(m_skeleton_esp_enabled), [] {
				hell::color_picker(xx("On Shot Skeleton Color"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_onshot_color)), 2, true);
				hell::color_picker(xx("On Shot Skeleton Color Outline"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_onshot_color_outline)), 3, true);
				hell::checkbox(xx("On shot skeleton"), VISUALS_PATH(m_skeleton_esp_onshot_enabled),
					[] {
						hell::slider_int(xx("Duration"), VISUALS_PATH(m_onshot_skeleton_duration), 1, 15, 0, false, xx("%ds"));
					}, 1);
				hell::color_picker(xx("Backtrack Skeleton Color"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_backtrack_color)), 2, true);
				hell::color_picker(xx("Backtrack Skeleton Color Outline"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_backtrack_color_outline)), 3, true);
				hell::checkbox(xx("Backtrack skeleton"), VISUALS_PATH(m_skeleton_esp_backtrack_enabled), [] {
					hell::combo(xx("Type"), GET_VAR(int, VISUALS_PATH(m_skeleton_esp_backtrack_type)), chams_type_bt_names, e_backtrack_cham_type::backtrack_cham_max);
				});
				});
			hell::color_picker(xx("OOF Arrows Color"), GET_VAR(hellcolor, VISUALS_PATH(m_colOOFArrows)), 1, true);
			hell::checkbox(xx("OOF Arrows"), VISUALS_PATH(m_bEnableOOFArrows));
			});

		float model_h = hell::calculate_child_height([] {
			hell::color_picker(xx("Visible chams color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_enemy_color)), 2, true);
		hell::color_picker(xx("Occluded chams color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_enemy_color_invis)), 3, true);
			hell::checkbox(xx("Chams"), VISUALS_PATH(m_chams_enabled), [] {
			hell::checkbox(xx("Ragdoll"), VISUALS_PATH(m_chams_enemy_ragdoll));
		hell::combo(xx("Visible material"), GET_VAR(int, VISUALS_PATH(m_chams_enemy_material)), chams_material, e_materials::material_max);
		hell::combo(xx("Occluded material"), GET_VAR(int, VISUALS_PATH(m_chams_enemy_material_invis)), chams_material, e_materials::material_max);
			if (GET_VAR(int, VISUALS_PATH(m_chams_enemy_material)) != e_materials::material_glow) {
				hell::checkbox(xx("Visible overlay"), VISUALS_PATH(m_glow_chams_enemy_visible));
			}
			if (GET_VAR(int, VISUALS_PATH(m_chams_enemy_material_invis)) != e_materials::material_glow) {
				hell::checkbox(xx("Occluded overlay"), VISUALS_PATH(m_glow_chams_enemy_invisible));
			}
			});
			hell::color_picker(xx("Enemy glow color"), GET_VAR(hellcolor, VISUALS_PATH(m_enemy_glow_color)), 1, true);
			hell::checkbox(xx("Glow"), VISUALS_PATH(m_enemy_glow), 0, 1);
			});

		ImGui::SetCursorPos({ col1_x, start_y });
		hell::child(xx("ESP"), { child_size.x, esp_h }, [] {
			hell::checkbox(xx("Enable"), VISUALS_PATH(m_esp_enabled));
			hell::color_picker(xx("Bounding Box Color"), GET_VAR(hellcolor, VISUALS_PATH(m_bb_fg_color)), 1, true);
			hell::color_picker(xx("Bounding Box Outline Color"), GET_VAR(hellcolor, VISUALS_PATH(m_bb_bg_color)), 2, true);
			hell::checkbox(xx("Bounding box"), VISUALS_PATH(m_bbox));
			hell::color_picker(xx("Name Color"), GET_VAR(hellcolor, VISUALS_PATH(m_name_color)), 1, true);
			hell::color_picker(xx("Name Color Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_name_color_bg)), 2, true);
			hell::checkbox(xx("Name"), VISUALS_PATH(m_name));
			hell::checkbox(xx("Steam avatar"), VISUALS_PATH(m_enabled_steam_avatar));
			hell::color_picker(xx("Health Bar Color"), GET_VAR(hellcolor, VISUALS_PATH(m_health_bar_color)), 1, true);
			hell::color_picker(xx("Health Bar Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_health_bar_color_bg)), 2, true);
			hell::checkbox(xx("Health bar"), VISUALS_PATH(m_health_bar));
			hell::color_picker(xx("Armor Bar Color"), GET_VAR(hellcolor, VISUALS_PATH(m_armor_bar_color)), 1, true);
			hell::color_picker(xx("Armor Bar Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_armor_bar_color_bg)), 2, true);
			hell::checkbox(xx("Armor bar"), VISUALS_PATH(m_armor_bar));
			hell::color_picker(xx("Item Text Color"), GET_VAR(hellcolor, VISUALS_PATH(m_weapon_name_color)), 1, true);
			hell::color_picker(xx("Item Text Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_weapon_name_color_bg)), 2, true);
			hell::checkbox(xx("Item text"), VISUALS_PATH(m_weapon_text));
			hell::color_picker(xx("Item Icon Color"), GET_VAR(hellcolor, VISUALS_PATH(m_weapon_icon_color)), 1, true);
			hell::checkbox(xx("Item icon"), VISUALS_PATH(m_weapon_icon));
			hell::color_picker(xx("Flags Color"), GET_VAR(hellcolor, VISUALS_PATH(m_esp_flags_color)), 8, true);
			hell::multi_combo(xx("Flags"), esp_flag_names, GET_VAR(std::vector<bool>, VISUALS_PATH(m_esp_flags)), e_esp_flags::esp_flag_max);
			hell::color_picker(xx("Skeleton ESP Color"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_color)), 2, true);
			hell::color_picker(xx("Skeleton ESP Outline Color"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_outline_color)), 3, true);
			hell::checkbox(xx("Skeleton ESP"), VISUALS_PATH(m_skeleton_esp_enabled), [] {
				hell::color_picker(xx("On Shot Skeleton Color"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_onshot_color)), 2, true);
				hell::color_picker(xx("On Shot Skeleton Color Outline"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_onshot_color_outline)), 3, true);
				hell::checkbox(xx("On shot skeleton"), VISUALS_PATH(m_skeleton_esp_onshot_enabled),
					[] {
						hell::slider_int(xx("Duration"), VISUALS_PATH(m_onshot_skeleton_duration), 1, 15, 0, false, xx("%ds"));
					}, 1);
				hell::color_picker(xx("History skeleton color"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_backtrack_color)), 2, true);
				hell::color_picker(xx("History skeleton Color Outline"), GET_VAR(hellcolor, VISUALS_PATH(m_skeleton_esp_backtrack_color_outline)), 3, true);
				hell::checkbox(xx("History skeleton"), VISUALS_PATH(m_skeleton_esp_backtrack_enabled), [] {
					hell::combo(xx("Type"), GET_VAR(int, VISUALS_PATH(m_skeleton_esp_backtrack_type)), chams_type_bt_names, e_backtrack_cham_type::backtrack_cham_max);
				});
				});
			hell::color_picker(xx("OOF Arrows color"), GET_VAR(hellcolor, VISUALS_PATH(m_colOOFArrows)), 1, true);
			hell::checkbox(xx("OOF Arrows"), VISUALS_PATH(m_bEnableOOFArrows));
			});

		ImGui::SetCursorPos({ col1_x, start_y + esp_h + spacing });
		hell::child(xx("Model"), { child_size.x, model_h }, [] {
			hell::color_picker(xx("Visible chams color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_enemy_color)), 2, true);
			hell::color_picker(xx("Occluded chams color"), GET_VAR(hellcolor, VISUALS_PATH(m_chams_enemy_color_invis)), 3, true);
			hell::checkbox(xx("Chams"), VISUALS_PATH(m_chams_enabled), [] {
			hell::checkbox(xx("Ragdoll"), VISUALS_PATH(m_chams_enemy_ragdoll));
			hell::combo(xx("Visible material"), GET_VAR(int, VISUALS_PATH(m_chams_enemy_material)), chams_material, e_materials::material_max);
			hell::combo(xx("Occluded material"), GET_VAR(int, VISUALS_PATH(m_chams_enemy_material_invis)), chams_material, e_materials::material_max);
				if (GET_VAR(int, VISUALS_PATH(m_chams_enemy_material)) != e_materials::material_glow) {
			hell::checkbox(xx("Visible overlay"), VISUALS_PATH(m_glow_chams_enemy_visible));
				}
				if (GET_VAR(int, VISUALS_PATH(m_chams_enemy_material_invis)) != e_materials::material_glow) {
					hell::checkbox(xx("Occluded overlay"), VISUALS_PATH(m_glow_chams_enemy_invisible));
				}
				});
			hell::color_picker(xx("Enemy Glow Color"), GET_VAR(hellcolor, VISUALS_PATH(m_enemy_glow_color)), 1, true);
			hell::checkbox(xx("Glow"), VISUALS_PATH(m_enemy_glow), 0, 1);
			});

		ImGui::SetCursorPos({ col2_x, start_y });
		{
			hellvec2 preview_child_size_rel = child_size;
			ImRect preview_child_size = { { ImGui::GetWindowPos() + ImGui::GetCursorPos() }, { ImGui::GetWindowPos() + ImGui::GetCursorPos() + preview_child_size_rel } };
			hell::child(xx("Preview"), { child_size.x, 382.f }, [&] {
				hellvec2 cursor_pos = ImGui::GetCursorPos();
				hellvec2 window_pos = ImGui::GetWindowPos();
				hellvec2 content_avail = ImGui::GetContentRegionAvail();

				c_visuals::bb_t preview_bb = {
						window_pos + cursor_pos,
						window_pos + cursor_pos + content_avail
				};

				hell::window::m_illegal_drag_rects[ImGui::GetID(xx("Preview"))] = preview_child_size;
				float preview_extra_x = (content_avail.x - 80.f) * 0.12f;
				preview_bb.shrink(40.f + preview_extra_x, 40.f);

				bool is_dragging = visual_preview::esp_preview(ImGui::GetWindowDrawList(), preview_bb);
				if (is_dragging) {
					ImGui::GetIO().WantCaptureMouse = false;
					ImGui::SetNextFrameWantCaptureMouse(false);
					hell::window::m_illegal_drag_rects[ImGui::GetID(xx("PreviewDrag"))] = ImRect(
						ImGui::GetWindowPos(),
						ImGui::GetWindowPos() + ImGui::GetWindowSize()
					);
				}
				ImGui::Dummy(content_avail);
				});
		}

		break;
	}
	case e_subtab::subtab_world: {
		float spacing = 18.f;
		float start_y = ImGui::GetCursorPosY();
		float col_spacing = ImGui::GetStyle().WindowPadding.x;

		float col1_x = ImGui::GetCursorPosX();
		float col2_x = col1_x + child_size.x + col_spacing;

		float general_h = hell::calculate_child_height([] {
			hell::multi_combo(xx("Removals"), removal_names, GET_VAR(std::vector<bool>, VISUALS_PATH(m_removals)), e_removals::removal_max);
			hell::slider_none(xx("Aspect ratio"), VISUALS_PATH(m_aspect_ratio), 0, 30, 0);
	
			});

		float overlay_h = hell::calculate_child_height([] {
			hell::color_picker(xx("World Hitmarkers Color"), GET_VAR(hellcolor, VISUALS_PATH(m_enabled_3d_hitmarkers_color)), 1, true);
			hell::checkbox(xx("World hitmarkers"), VISUALS_PATH(m_enabled_3d_hitmarkers));
			hell::color_picker(xx("2D Hitmarker Color"), GET_VAR(hellcolor, MISC_PATH(m_colHitMarker2d)), 1, true);
			hell::checkbox(xx("2D htmarker"), MISC_PATH(m_bHitMarker2d));
			hell::color_picker(xx("World Damage Markers Color"), GET_VAR(hellcolor, VISUALS_PATH(m_enabled_3d_damage_markers_color)), 1, true);
			hell::checkbox(xx("Damage markers"), VISUALS_PATH(m_enabled_3d_damage_markers));
			hell::color_picker(xx("Visualize Aimbot Color"), GET_VAR(hellcolor, VISUALS_PATH(m_visualize_hitbox_color)), 2, true);
			hell::checkbox(xx("Visualize aimbot"), VISUALS_PATH(m_visualize_hitboxes), [] {
				hell::slider_int(xx("Field Of View"), VISUALS_PATH(m_visualize_hitbox_fov), 0, 100, 0, false, xx("%ddeg"));
				});
			});
		
		float modulation_h = hell::calculate_child_height([] {
			hell::color_picker(xx("World Color"), GET_VAR(hellcolor, VISUALS_PATH(m_world_color)), 1, true);
			hell::checkbox(xx("World"), VISUALS_PATH(m_enable_world_modulation));
			hell::color_picker(xx("Lightnings Color"), GET_VAR(hellcolor, VISUALS_PATH(m_lightings_color)), 1, true);
			hell::checkbox(xx("Lightning"), VISUALS_PATH(m_enable_lightnings_modulation));
			hell::color_picker(xx("Sky Color"), GET_VAR(hellcolor, VISUALS_PATH(m_sky_color)), 1, true);
			hell::color_picker(xx("Clouds Color"), GET_VAR(hellcolor, VISUALS_PATH(m_clouds_color)), 2, true);
			hell::color_picker(xx("Sun Color"), GET_VAR(hellcolor, VISUALS_PATH(m_sun_color)), 3, true);
			hell::checkbox(xx("Sky Modulation"), VISUALS_PATH(m_enable_sky_modulation));
			hell::checkbox(xx("Custom skybox"), VISUALS_PATH(m_enable_custom_sky), [] {
				hell::combo(xx("Type"), GET_VAR(int, VISUALS_PATH(m_selected_skybox)), arr_skybox_names, 14);
				});

	
	

			hell::checkbox(xx("World blur"), VISUALS_PATH(m_enable_world_blur), [] {
				hell::slider_float(xx("Near start"), VISUALS_PATH(m_world_blur_near_start), -5000.f, 5000.f, 0, false, xx("%.1f"));
				hell::slider_float(xx("Near end"), VISUALS_PATH(m_world_blur_near_end), -5000.f, 5000.f, 0, false, xx("%.1f"));
				hell::slider_float(xx("Far start"), VISUALS_PATH(m_world_blur_far_start), -5000.f, 5000.f, 0, false, xx("%.1f"));
				hell::slider_float(xx("Far end"), VISUALS_PATH(m_world_blur_far_end), -5000.f, 5000.f, 0, false, xx("%.1f"));
				});



			hell::slider_none(xx("Brightness"), VISUALS_PATH(m_world_exposure), 0, 200, 0, xx("%d%%"));
			});

		ImGui::SetCursorPos({ col1_x, start_y });
		hell::child(xx("General"), { child_size.x, general_h }, [] {
			hell::multi_combo(xx("Removals"), removal_names, GET_VAR(std::vector<bool>, VISUALS_PATH(m_removals)), e_removals::removal_max);
			hell::slider_none(xx("Aspect ratio"), VISUALS_PATH(m_aspect_ratio), 0, 30, 0);

			}
		);

		ImGui::SetCursorPos({ col2_x, start_y });
		hell::child(xx("Overlay"), { child_size.x, overlay_h }, [] {
			hell::color_picker(xx("Overlay Color"), GET_VAR(hellcolor, VISUALS_PATH(m_overlay_color)), 1, true);
			hell::color_picker(xx("World Hitmarkers Color"), GET_VAR(hellcolor, VISUALS_PATH(m_enabled_3d_hitmarkers_color)), 1, true);
			hell::checkbox(xx("World hitmarkers"), VISUALS_PATH(m_enabled_3d_hitmarkers));
			hell::color_picker(xx("2D Hitmarker Color"), GET_VAR(hellcolor, MISC_PATH(m_colHitMarker2d)), 1, true);
			hell::checkbox(xx("2D hitmarker"), MISC_PATH(m_bHitMarker2d));
			hell::color_picker(xx("World Damage Markers Color"), GET_VAR(hellcolor, VISUALS_PATH(m_enabled_3d_damage_markers_color)), 1, true);
			hell::checkbox(xx("Damage markers"), VISUALS_PATH(m_enabled_3d_damage_markers));
			hell::color_picker(xx("Visualize Aimbot Color"), GET_VAR(hellcolor, VISUALS_PATH(m_visualize_hitbox_color)), 1, true);
			hell::checkbox(xx("Visualize aimbot"), VISUALS_PATH(m_visualize_hitboxes), [] {
				hell::slider_int(xx("Field of view"), VISUALS_PATH(m_visualize_hitbox_fov), 0, 100, 0, false, xx("%ddeg"));
				});
			}
		);

		bool modulation_under_first = general_h <= overlay_h;
		ImGui::SetCursorPos({ modulation_under_first ? col1_x : col2_x, start_y + (modulation_under_first ? general_h : overlay_h) + spacing });
		hell::child(xx("Modulation"), { child_size.x, modulation_h }, [] {
			hell::color_picker(xx("World Color"), GET_VAR(hellcolor, VISUALS_PATH(m_world_color)), 1, true);
			hell::checkbox(xx("World"), VISUALS_PATH(m_enable_world_modulation));

			hell::color_picker(xx("Lightnings Color"), GET_VAR(hellcolor, VISUALS_PATH(m_lightings_color)), 1, true);
			hell::checkbox(xx("Lightning"), VISUALS_PATH(m_enable_lightnings_modulation));

			hell::color_picker(xx("Sky Color"), GET_VAR(hellcolor, VISUALS_PATH(m_sky_color)), 1, true);
			hell::color_picker(xx("Clouds Color"), GET_VAR(hellcolor, VISUALS_PATH(m_clouds_color)), 2, true);
			hell::color_picker(xx("Sun Color"), GET_VAR(hellcolor, VISUALS_PATH(m_sun_color)), 3, true);
			hell::checkbox(xx("Sky modulation"), VISUALS_PATH(m_enable_sky_modulation));
			hell::checkbox(xx("Custom skybox"), VISUALS_PATH(m_enable_custom_sky), [] {
				hell::combo(xx("Type"), GET_VAR(int, VISUALS_PATH(m_selected_skybox)), arr_skybox_names, 14);
				});

			hell::checkbox(xx("World Blur"), VISUALS_PATH(m_enable_world_blur), [] {
				hell::slider_float(xx("Near start"), VISUALS_PATH(m_world_blur_near_start), -5000.f, 5000.f, 0, false, xx("%.1f"));
				hell::slider_float(xx("Near end"), VISUALS_PATH(m_world_blur_near_end), -5000.f, 5000.f, 0, false, xx("%.1f"));
				hell::slider_float(xx("Far start"), VISUALS_PATH(m_world_blur_far_start), -5000.f, 5000.f, 0, false, xx("%.1f"));
				hell::slider_float(xx("Far end"), VISUALS_PATH(m_world_blur_far_end), -5000.f, 5000.f, 0, false, xx("%.1f"));
				});


			hell::slider_none(xx("Brightness"), VISUALS_PATH(m_world_exposure), 0, 200, 0, xx("%d%%"));
			}
		);

		break;
	}
	case e_subtab::subtab_other: {
		float spacing = 18.f;
		float start_y = ImGui::GetCursorPosY();
		float col_spacing = ImGui::GetStyle().WindowPadding.x;

		float col1_x = ImGui::GetCursorPosX();
		float col2_x = col1_x + child_size.x + col_spacing;

		float dropped_items_h = hell::calculate_child_height([] {
			hell::color_picker(xx("Items Names Color"), GET_VAR(hellcolor, VISUALS_PATH(m_dropped_items_names_color)), 2, true);
			hell::color_picker(xx("Items Names Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_dropped_items_names_color_bg)), 3, true);
			hell::checkbox(xx("Items names"), VISUALS_PATH(m_dropped_items_names), [] {
				hell::combo(xx("Font"), GET_VAR(int, VISUALS_PATH(m_dropped_items_names_font_type)), font_names, IM_ARRAYSIZE(font_names));
				hell::combo(xx("Shadow"), GET_VAR(int, VISUALS_PATH(m_dropped_items_names_shadow_type)), shadow_names, IM_ARRAYSIZE(shadow_names));
				});
			hell::color_picker(xx("Items Icons Color"), GET_VAR(hellcolor, VISUALS_PATH(m_dropped_items_icons_color)), 1, true);
			hell::checkbox(xx("Items icons"), VISUALS_PATH(m_dropped_items_icons));
			hell::slider_int(xx("Distance"), VISUALS_PATH(m_dropped_items_distance), 100, 1000, 0, false, xx("%du"));
			});

		float grenades_info_h = hell::calculate_child_height([] {
			hell::color_picker(xx("Inferno Radius Color"), GET_VAR(hellcolor, VISUALS_PATH(m_inferno_radius_color)), 2, true);
			hell::checkbox(xx("Inferno radius"), VISUALS_PATH(m_enable_inferno_radius), [] {
				hell::slider_int(xx("From middle alpha"), VISUALS_PATH(m_from_middle_alpha), 0, 255, 0, false);
				hell::slider_int(xx("To middle alpha"), VISUALS_PATH(m_to_middle_alpha), 0, 255, 0, false);
				});
			hell::color_picker(xx("Smoke radius color"), GET_VAR(hellcolor, VISUALS_PATH(m_smoke_radius_color)), 2, true);
			hell::checkbox(xx("Smoke radius"), VISUALS_PATH(m_enable_smoke_radius), [] {
				hell::slider_int(xx("Smoke from middle alpha"), VISUALS_PATH(m_smoke_from_middle_alpha), 0, 255, 0, false);
				hell::slider_int(xx("Smoke to middle alpha"), VISUALS_PATH(m_smoke_to_middle_alpha), 0, 255, 0, false);
				});
			hell::color_picker(xx("Grenade prediction color"), GET_VAR(hellcolor, VISUALS_PATH(m_grenade_prediction_color)), 2, true);
			hell::checkbox(xx("Grenade prediction"), VISUALS_PATH(m_enable_grenade_prediction), [] {
				hell::checkbox(xx("Pulsating"), VISUALS_PATH(m_grenade_prediction_pulsating));
				hell::checkbox(xx("Glow"), VISUALS_PATH(m_grenade_prediction_glow));
				});
			hell::color_picker(xx("Proximity Warnings Color"), GET_VAR(hellcolor, VISUALS_PATH(m_proximity_warnings_color)), 1, true);
			hell::checkbox(xx("Grenade proximity warning"), VISUALS_PATH(m_enable_proximity_warnings));
			hell::color_picker(xx("Grenade Names Color"), GET_VAR(hellcolor, VISUALS_PATH(m_grenade_names_color)), 2, true);
			hell::color_picker(xx("Grenade Names Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_grenade_names_color_bg)), 3, true);
			hell::checkbox(xx("Grenade names"), VISUALS_PATH(m_enable_grenade_names), [] {
				hell::checkbox(xx("Show icon"), VISUALS_PATH(m_grenade_names_show_icon));
				hell::combo(xx("Font"), GET_VAR(int, VISUALS_PATH(m_grenade_names_font_type)), font_names, IM_ARRAYSIZE(font_names));
				hell::combo(xx("Shadow"), GET_VAR(int, VISUALS_PATH(m_grenade_names_shadow_type)), shadow_names, IM_ARRAYSIZE(shadow_names));
				});
			});

		float planted_bomb_h = hell::calculate_child_height([] {
			hell::checkbox(xx("Bomb overlay"), VISUALS_PATH(m_bomb_hud_enabled));
			hell::color_picker(xx("World Icon Color"), GET_VAR(hellcolor, VISUALS_PATH(m_planted_bomb_icon_color)), 1, true);
			hell::checkbox(xx("World icon"), VISUALS_PATH(m_planted_bomb_world_icon));
			hell::color_picker(xx("World Timer Color"), GET_VAR(hellcolor, VISUALS_PATH(m_planted_bomb_timer_color)), 2, true);
			hell::color_picker(xx("World Timer Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_planted_bomb_timer_color_bg)), 3, true);
			hell::checkbox(xx("World timer"), VISUALS_PATH(m_planted_bomb_world_timer), [] {
				hell::combo(xx("Font"), GET_VAR(int, VISUALS_PATH(m_planted_bomb_timer_font_type)), font_names, IM_ARRAYSIZE(font_names));
				hell::combo(xx("Shadow"), GET_VAR(int, VISUALS_PATH(m_planted_bomb_timer_shadow_type)), shadow_names, IM_ARRAYSIZE(shadow_names));
				});
			hell::slider_int(xx("Distance"), VISUALS_PATH(m_planted_bomb_distance), 100, 1000, 0, false, xx("%du"));
			});

		ImGui::SetCursorPos({ col1_x, start_y });
		hell::child(xx("Dropped items"), { child_size.x, dropped_items_h }, [] {
			hell::color_picker(xx("Items Names Color"), GET_VAR(hellcolor, VISUALS_PATH(m_dropped_items_names_color)), 2, true);
			hell::color_picker(xx("Items Names Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_dropped_items_names_color_bg)), 3, true);
			hell::checkbox(xx("Items names"), VISUALS_PATH(m_dropped_items_names), [] {
				hell::combo(xx("Font"), GET_VAR(int, VISUALS_PATH(m_dropped_items_names_font_type)), font_names, IM_ARRAYSIZE(font_names));
				hell::combo(xx("Shadow"), GET_VAR(int, VISUALS_PATH(m_dropped_items_names_shadow_type)), shadow_names, IM_ARRAYSIZE(shadow_names));
				});
			hell::color_picker(xx("Items icons Color"), GET_VAR(hellcolor, VISUALS_PATH(m_dropped_items_icons_color)), 1, true);
			hell::checkbox(xx("Items Icons"), VISUALS_PATH(m_dropped_items_icons));
			hell::slider_int(xx("Distance"), VISUALS_PATH(m_dropped_items_distance), 100, 1000, 0, false, xx("%du"));
			}
		);

		ImGui::SetCursorPos({ col2_x, start_y });
		hell::child(xx("Grenades info"), { child_size.x, grenades_info_h }, [] {
			hell::color_picker(xx("Inferno Radius Color"), GET_VAR(hellcolor, VISUALS_PATH(m_inferno_radius_color)), 2, true);
			hell::checkbox(xx("Inferno radius"), VISUALS_PATH(m_enable_inferno_radius), [] {
				hell::slider_int(xx("From middle alpha"), VISUALS_PATH(m_from_middle_alpha), 0, 255, 0, false);
				hell::slider_int(xx("To middle alpha"), VISUALS_PATH(m_to_middle_alpha), 0, 255, 0, false);
				});
			hell::color_picker(xx("Smoke radius color"), GET_VAR(hellcolor, VISUALS_PATH(m_smoke_radius_color)), 2, true);
			hell::checkbox(xx("Smoke radius"), VISUALS_PATH(m_enable_smoke_radius), [] {
				hell::slider_int(xx("Smoke from middle alpha"), VISUALS_PATH(m_smoke_from_middle_alpha), 0, 255, 0, false);
				hell::slider_int(xx("Smoke to middle alpha"), VISUALS_PATH(m_smoke_to_middle_alpha), 0, 255, 0, false);
				});
			hell::color_picker(xx("Grenade prediction color"), GET_VAR(hellcolor, VISUALS_PATH(m_grenade_prediction_color)), 2, true);
			hell::checkbox(xx("Grenade prediction"), VISUALS_PATH(m_enable_grenade_prediction), [] {
				hell::checkbox(xx("Pulsating"), VISUALS_PATH(m_grenade_prediction_pulsating));
				hell::checkbox(xx("Glow"), VISUALS_PATH(m_grenade_prediction_glow));
				});
			hell::color_picker(xx("Proximity Warnings Color"), GET_VAR(hellcolor, VISUALS_PATH(m_proximity_warnings_color)), 1, true);
			hell::checkbox(xx("Grenade proximity warning"), VISUALS_PATH(m_enable_proximity_warnings));
			hell::color_picker(xx("Grenade Names Color"), GET_VAR(hellcolor, VISUALS_PATH(m_grenade_names_color)), 2, true);
			hell::color_picker(xx("Grenade Names Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_grenade_names_color_bg)), 3, true);
			hell::checkbox(xx("Grenade names"), VISUALS_PATH(m_enable_grenade_names), [] {
				hell::checkbox(xx("Show icon"), VISUALS_PATH(m_grenade_names_show_icon));
				hell::combo(xx("Font"), GET_VAR(int, VISUALS_PATH(m_grenade_names_font_type)), font_names, IM_ARRAYSIZE(font_names));
				hell::combo(xx("Shadow"), GET_VAR(int, VISUALS_PATH(m_grenade_names_shadow_type)), shadow_names, IM_ARRAYSIZE(shadow_names));
				});
			}
		);

		bool bomb_under_first = dropped_items_h <= grenades_info_h;
		ImGui::SetCursorPos({ bomb_under_first ? col1_x : col2_x, start_y + (bomb_under_first ? dropped_items_h : grenades_info_h) + spacing });
		hell::child(xx("Planted Bomb"), { child_size.x, planted_bomb_h }, [] {
			hell::checkbox(xx("Bomb overlay"), VISUALS_PATH(m_bomb_hud_enabled));
			hell::color_picker(xx("World Icon Color"), GET_VAR(hellcolor, VISUALS_PATH(m_planted_bomb_icon_color)), 1, true);
			hell::checkbox(xx("World icon"), VISUALS_PATH(m_planted_bomb_world_icon));
			hell::color_picker(xx("World Timer Color"), GET_VAR(hellcolor, VISUALS_PATH(m_planted_bomb_timer_color)), 2, true);
			hell::color_picker(xx("World Timer Color BG"), GET_VAR(hellcolor, VISUALS_PATH(m_planted_bomb_timer_color_bg)), 3, true);
			hell::checkbox(xx("World timer"), VISUALS_PATH(m_planted_bomb_world_timer), [] {
				hell::combo(xx("Font"), GET_VAR(int, VISUALS_PATH(m_planted_bomb_timer_font_type)), font_names, IM_ARRAYSIZE(font_names));
				hell::combo(xx("Shadow"), GET_VAR(int, VISUALS_PATH(m_planted_bomb_timer_shadow_type)), shadow_names, IM_ARRAYSIZE(shadow_names));
				});
			hell::slider_int(xx("Distance"), VISUALS_PATH(m_planted_bomb_distance), 100, 1000, 0, false, xx("%du"));
			}
		);


		break;
	}
	}
}
