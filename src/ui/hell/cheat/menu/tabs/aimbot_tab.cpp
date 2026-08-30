#include "aimbot_tab.h"
#include <cheat/menu/hell_gui/hell_gui.h>
#include <includes.h>
#include <cheat/config/vars.h>

#include <vector>     
#include <algorithm>  
#include <string>     
#include <context.h>



const char* weapon_names[] = {
	xx("Light Pistol"), // 0
	xx("Deagle"),       // 1
	xx("Revolver"),
	xx("Smg"),
	xx("Lmg"),
	xx("Ar"),
	xx("Shotgun"),
	xx("Scout"),
	xx("Autosniper"), // 8
	xx("Awp") // 9
};


const char* hitbox_names[] = {
	xx("Head"),
	xx("Neck"),
	xx("Chest"),
	xx("Stomach"),
	xx("Pelvis"),
	xx("Arms"),
	xx("Legs"),
	xx("Feet")
};

const char* targetting_types[] = {
	xx("Damage"),
	xx("Hitchance"),
	xx("Health")
};

const char* hitbox_preference_types[] = {
	xx("Damage"),
	xx("Hitchance")
};

const char* autostop_types[] = {
	xx("Early"),
	xx("In air")
};

static std::string get_preview_from_enums(const std::vector<int>& hitbox_enums) {
	if (hitbox_enums.empty()) {
		return xx("None");
	}
	std::string preview_text;
	for (size_t i = 0; i < hitbox_enums.size(); ++i) {
		preview_text += hitbox_names[hitbox_enums[i]];
		if (i < hitbox_enums.size() - 1) {
			preview_text += ", ";
		}
	}
	return preview_text;
}


static bool begin_config_change_scope() {
	g_ctx->m_rage_config_needs_update = false;
	return false;
}

static void end_config_change_scope(bool& dirty_flag) {
	if (dirty_flag)
		g_ctx->m_rage_config_needs_update = true;
}

fnv1a_t tabs::aimbot::get_min_damage_holder_id(int current_weapon) {
	switch (current_weapon) {
	case e_ragebot_weapons::weapon_light_pistol:
		return RAGEBOT_PATH(m_mindamage_light_pistol);
	case e_ragebot_weapons::weapon_deagle:
		return RAGEBOT_PATH(m_mindamage_deagle);
	case e_ragebot_weapons::weapon_revolver:
		return RAGEBOT_PATH(m_mindamage_revolver);
	case e_ragebot_weapons::weapon_smg:
		return RAGEBOT_PATH(m_mindamage_smg);
	case e_ragebot_weapons::weapon_lmg:
		return RAGEBOT_PATH(m_mindamage_lmg);
	case e_ragebot_weapons::weapon_ar:
		return RAGEBOT_PATH(m_mindamage_ar);
	case e_ragebot_weapons::weapon_shotgun:
		return RAGEBOT_PATH(m_mindamage_shotgun);
	case e_ragebot_weapons::weapon_scout:
		return RAGEBOT_PATH(m_mindamage_scout);
	case e_ragebot_weapons::weapon_autosniper:
		return RAGEBOT_PATH(m_mindamage_autosniper);
	case e_ragebot_weapons::weapon_awp:
		return RAGEBOT_PATH(m_mindamage_awp);
	default:
		return RAGEBOT_PATH(m_mindamage_light_pistol);
	}
}
fnv1a_t tabs::aimbot::get_air_hitchance_holder_id(int current_weapon) {
	switch (current_weapon) {
	case e_ragebot_weapons::weapon_light_pistol:
		return RAGEBOT_PATH(m_force_hitchance_light_pistol);

	case e_ragebot_weapons::weapon_deagle:
		return RAGEBOT_PATH(m_force_hitchance_deagle);

	case e_ragebot_weapons::weapon_revolver:
		return RAGEBOT_PATH(m_force_hitchance_revolver);

	case e_ragebot_weapons::weapon_smg:
		return RAGEBOT_PATH(m_force_hitchance_smg);

	case e_ragebot_weapons::weapon_lmg:
		return RAGEBOT_PATH(m_force_hitchance_lmg);

	case e_ragebot_weapons::weapon_ar:
		return RAGEBOT_PATH(m_force_hitchance_ar);

	case e_ragebot_weapons::weapon_shotgun:
		return RAGEBOT_PATH(m_force_hitchance_shotgun);

	case e_ragebot_weapons::weapon_scout:
		return RAGEBOT_PATH(m_force_hitchance_scout);

	case e_ragebot_weapons::weapon_autosniper:
		return RAGEBOT_PATH(m_force_hitchance_autosniper);

	case e_ragebot_weapons::weapon_awp:
		return RAGEBOT_PATH(m_force_hitchance_awp);

	default:
		return RAGEBOT_PATH(m_force_hitchance_light_pistol);
	}
}
fnv1a_t tabs::aimbot::get_hitchance_holder_id(int current_weapon) {
	switch (current_weapon) {
	case e_ragebot_weapons::weapon_light_pistol:
		return RAGEBOT_PATH(m_hitchance_light_pistol);
	case e_ragebot_weapons::weapon_deagle:
		return RAGEBOT_PATH(m_hitchance_deagle);
	case e_ragebot_weapons::weapon_revolver:
		return RAGEBOT_PATH(m_hitchance_revolver);
	case e_ragebot_weapons::weapon_smg:
		return RAGEBOT_PATH(m_hitchance_smg);
	case e_ragebot_weapons::weapon_lmg:
		return RAGEBOT_PATH(m_hitchance_lmg);
	case e_ragebot_weapons::weapon_ar:
		return RAGEBOT_PATH(m_hitchance_ar);
	case e_ragebot_weapons::weapon_shotgun:
		return RAGEBOT_PATH(m_hitchance_shotgun);
	case e_ragebot_weapons::weapon_scout:
		return RAGEBOT_PATH(m_hitchance_scout);
	case e_ragebot_weapons::weapon_autosniper:
		return RAGEBOT_PATH(m_hitchance_autosniper);
	case e_ragebot_weapons::weapon_awp:
		return RAGEBOT_PATH(m_hitchance_awp);
	default:
		return RAGEBOT_PATH(m_hitchance_light_pistol);
	}
}

fnv1a_t tabs::aimbot::get_pointscale_holder_id(int current_weapon) {
	switch (current_weapon) {
	case e_ragebot_weapons::weapon_light_pistol:
		return RAGEBOT_PATH(m_pointscale_light_pistol);
	case e_ragebot_weapons::weapon_deagle:
		return RAGEBOT_PATH(m_pointscale_deagle);
	case e_ragebot_weapons::weapon_revolver:
		return RAGEBOT_PATH(m_pointscale_revolver);
	case e_ragebot_weapons::weapon_smg:
		return RAGEBOT_PATH(m_pointscale_smg);
	case e_ragebot_weapons::weapon_lmg:
		return RAGEBOT_PATH(m_pointscale_lmg);
	case e_ragebot_weapons::weapon_ar:
		return RAGEBOT_PATH(m_pointscale_ar);
	case e_ragebot_weapons::weapon_shotgun:
		return RAGEBOT_PATH(m_pointscale_shotgun);
	case e_ragebot_weapons::weapon_scout:
		return RAGEBOT_PATH(m_pointscale_scout);
	case e_ragebot_weapons::weapon_autosniper:
		return RAGEBOT_PATH(m_pointscale_autosniper);
	case e_ragebot_weapons::weapon_awp:
		return RAGEBOT_PATH(m_pointscale_awp);
	default:
		return RAGEBOT_PATH(m_pointscale_light_pistol);
	}
}

void tabs::render_aimbot() {
	static int current_weapon = e_ragebot_weapons::weapon_light_pistol;
	bool changed = begin_config_change_scope();

	hell::set_cursor_pos_relative_y();

	hellvec2 col_size = hell::calculate_column_size(2);
	float spacing = 18.f;
	float start_y = ImGui::GetCursorPosY();
	float col_spacing = ImGui::GetStyle().WindowPadding.x;

	float col1_x = ImGui::GetCursorPosX();
	float col2_x = col1_x + col_size.x + col_spacing;

	float general_h = hell::calculate_child_height([&] {
		changed |= hell::checkbox(xx("Enable"), RAGEBOT_PATH(m_enabled_ragebot));
		changed |= hell::checkbox(xx("Automatic fire"), RAGEBOT_PATH(m_autofire));
		changed |= hell::checkbox(xx("Extrapolation"), RAGEBOT_PATH(m_enabled_extrapolation), [&] {
			changed |= hell::slider_int(
				xx("Extrapolation ticks"),
				RAGEBOT_PATH(m_max_extrapolation_ticks), 1, 15);

			});
		changed |= hell::checkbox(xx("Silent aimbot"), RAGEBOT_PATH(m_silent_aim));
		changed |= hell::checkbox(xx("Auto scope"), RAGEBOT_PATH(m_auto_scope));
		changed |= hell::color_picker(xx("Auto retract color"), GET_VAR(hellcolor, MISC_PATH(m_auto_peek_color)), 1, true);
		changed |= hell::checkbox(xx("Auto retreat"), MISC_PATH(m_enabled_auto_peek));
		changed |= hell::checkbox(xx("Zeusbot"), RAGEBOT_PATH(m_taser_bot));
		});

	float weapon_h = hell::calculate_child_height([&] {
		changed |= hell::combo(xx("Weapon group"), current_weapon, weapon_names, e_ragebot_weapons::weapon_not_wanted);
		
		changed |= hell::slider_int(xx("Point scale"), tabs::aimbot::get_pointscale_holder_id(current_weapon), 1, 100, 0, false, xx("%d%%"));
		changed |= hell::min_dmg_slider(xx("Minimum damage"), tabs::aimbot::get_min_damage_holder_id(current_weapon), 1, 125);
		changed |= hell::slider_int(xx("Hit chance"), tabs::aimbot::get_hitchance_holder_id(current_weapon), 1, 100, 0, false, xx("%d%%"));
		});

	float accuracy_h = hell::calculate_child_height([&] {
		changed |= hell::checkbox(xx("Auto stop"), RAGEBOT_PATH(m_autostop), [] {
			hell::multi_combo(xx("Mode"), autostop_types, GET_VAR_VEC(bool, RAGEBOT_PATH(m_autostop_modes)), e_ragebot_autostop_types::rage_autostop_max);
			});
	
		changed |= hell::checkbox(xx("Auto wall"), RAGEBOT_PATH(m_autowall));
		changed |= hell::checkbox(xx("Force shoot"), RAGEBOT_PATH(m_force_shoot), [&] {
			changed |= hell::slider_int(
				xx("Air hit chance"),
				tabs::aimbot::get_air_hitchance_holder_id(current_weapon),
				1,
				100,
				0,
				false,
				xx("%d%%")
			);
			});

		auto& selected_hitboxes = GET_VAR(std::vector<std::vector<int>>, RAGEBOT_PATH(m_selected_hitboxes)).at(current_weapon);
		auto& multipoint_hitboxes = GET_VAR(std::vector<std::vector<int>>, RAGEBOT_PATH(m_multipoint_hitboxes)).at(current_weapon);

		std::string hitbox_preview = get_preview_from_enums(selected_hitboxes);
		if (hell::begin_combo(xx("Hitboxes"), hitbox_preview.c_str())) {
			for (int i = 0; i < e_ragebot_hitboxes::rage_hitbox_max; ++i) {
				auto it = std::find(selected_hitboxes.begin(), selected_hitboxes.end(), i);
				bool is_selected = (it != selected_hitboxes.end());

				if (hell::selectable(hitbox_names[i], is_selected)) {
					if (is_selected) {
						selected_hitboxes.erase(it);
						auto mp_it = std::find(multipoint_hitboxes.begin(), multipoint_hitboxes.end(), i);
						if (mp_it != multipoint_hitboxes.end()) {
							multipoint_hitboxes.erase(mp_it);
						}
					}
					else {
						selected_hitboxes.push_back(i);
						std::sort(selected_hitboxes.begin(), selected_hitboxes.end());
					}
					changed = true;
				}
			}
			hell::end_combo();
		}

		std::string multipoint_preview = get_preview_from_enums(multipoint_hitboxes);
		if (hell::begin_combo(xx("Multipoint hitboxes"), multipoint_preview.c_str())) {
			if (selected_hitboxes.empty()) {
				hell::label(xx("Select hitboxes first"), { 255, 255, 255, 200 });
			}
			else {
				for (const auto& hitbox_enum : selected_hitboxes) {
					auto it = std::find(multipoint_hitboxes.begin(), multipoint_hitboxes.end(), hitbox_enum);
					bool is_selected = (it != multipoint_hitboxes.end());

					if (hell::selectable(hitbox_names[hitbox_enum], is_selected)) {
						if (is_selected) {
							multipoint_hitboxes.erase(it);
						}
						else {
							multipoint_hitboxes.push_back(hitbox_enum);
							std::sort(multipoint_hitboxes.begin(), multipoint_hitboxes.end());
						}
						changed = true;
					}
				}
			}
			hell::end_combo();
		}
		});

	float antiaim_h = hell::calculate_child_height([] {
		hell::checkbox(xx("Enable"), ANTIAIM_PATH(m_enabled_antiaim), NULL, 1);
		hell::checkbox(xx("At targets"), ANTIAIM_PATH(m_at_target));
		hell::slider_int(xx("Yaw"), ANTIAIM_PATH(m_yaw), -180, 180);
		hell::slider_int(xx("Pitch"), ANTIAIM_PATH(m_pitch), -90, 90);
		hell::checkbox(xx("Yaw jitter"), ANTIAIM_PATH(m_yaw_jitter), [] {
			hell::slider_int(xx("Amount"), ANTIAIM_PATH(m_yaw_jitter_amount), -180, 180);
			});
		hell::checkbox(xx("Pitch jitter"), ANTIAIM_PATH(m_pitch_jitter), [] {
			hell::slider_int(xx("Amount"), ANTIAIM_PATH(m_pitch_jitter_amount), -89, 89);
			});
		hell::checkbox(xx("Hide shots"), ANTIAIM_PATH(m_hide_shots));
		hell::checkbox(xx("Manual left"), ANTIAIM_PATH(m_override_left));
		hell::checkbox(xx("Manual right"), ANTIAIM_PATH(m_override_right));
		});

	ImGui::SetCursorPos({ col1_x, start_y });
	hell::child(xx("General"), { col_size.x, general_h }, [&] {
		changed |= hell::checkbox(xx("Enabled"), RAGEBOT_PATH(m_enabled_ragebot));
		changed |= hell::checkbox(xx("Automatic fire"), RAGEBOT_PATH(m_autofire));
		changed |= hell::checkbox(xx("Extrapolation"), RAGEBOT_PATH(m_enabled_extrapolation ), [&] {
			changed |= hell::slider_int(
				xx("Extrapolation ticks"),
				RAGEBOT_PATH(m_max_extrapolation_ticks),1,8);
				
			});
		changed |= hell::checkbox(xx("Silent aimbot"), RAGEBOT_PATH(m_silent_aim));
		changed |= hell::checkbox(xx("Auto scope"), RAGEBOT_PATH(m_auto_scope));
		changed |= hell::color_picker(xx("Auto retract color"), GET_VAR(hellcolor, MISC_PATH(m_auto_peek_color)), 1, true);
		changed |= hell::checkbox(xx("Auto retreat"), MISC_PATH(m_enabled_auto_peek));
		changed |= hell::checkbox(xx("Zeusbot"), RAGEBOT_PATH(m_taser_bot));
		});

	ImGui::SetCursorPos({ col1_x, start_y + general_h + spacing });
	hell::child(xx("Anti-aim"), { col_size.x, antiaim_h }, [&] {
		hell::checkbox(xx("Enable"), ANTIAIM_PATH(m_enabled_antiaim), NULL, 1);
		hell::checkbox(xx("At target"), ANTIAIM_PATH(m_at_target));
		hell::slider_int(xx("Yaw"), ANTIAIM_PATH(m_yaw), -180, 180);
		hell::slider_int(xx("Pitch"), ANTIAIM_PATH(m_pitch), -89, 89);
		hell::checkbox(xx("Yaw jitter"), ANTIAIM_PATH(m_yaw_jitter), [] {
			hell::slider_int(xx("Amount"), ANTIAIM_PATH(m_yaw_jitter_amount), -180, 180);
			});
		hell::checkbox(xx("Pitch jitter"), ANTIAIM_PATH(m_pitch_jitter), [] {
			hell::slider_int(xx("Amount"), ANTIAIM_PATH(m_pitch_jitter_amount), -89, 89);
			});
		hell::checkbox(xx("Hide shots"), ANTIAIM_PATH(m_hide_shots));
		hell::checkbox(xx("Manual left"), ANTIAIM_PATH(m_override_left));
		hell::checkbox(xx("Manual right"), ANTIAIM_PATH(m_override_right));
		});

	ImGui::SetCursorPos({ col2_x, start_y });
	hell::child(xx("Weapon"), { col_size.x, weapon_h }, [&] {
		changed |= hell::combo(xx("Weapon group"), current_weapon, weapon_names, e_ragebot_weapons::weapon_not_wanted);

		changed |= hell::slider_int(xx("Point scale"), tabs::aimbot::get_pointscale_holder_id(current_weapon), 1, 100, 0, false, xx("%d%%"));
		changed |= hell::min_dmg_slider(xx("Minimum damage"), tabs::aimbot::get_min_damage_holder_id(current_weapon), 1, 125);
		changed |= hell::slider_int(xx("Hit chance"), tabs::aimbot::get_hitchance_holder_id(current_weapon), 1, 100, 0, false, xx("%d%%"));
		});

	ImGui::SetCursorPos({ col2_x, start_y + weapon_h + spacing });
	hell::child(xx("Accuracy"), { col_size.x, accuracy_h }, [&] {
		changed |= hell::checkbox(xx("Auto stop"), RAGEBOT_PATH(m_autostop), [] {
			hell::multi_combo(xx("Mode"), autostop_types, GET_VAR_VEC(bool, RAGEBOT_PATH(m_autostop_modes)), e_ragebot_autostop_types::rage_autostop_max);
			});

		changed |= hell::checkbox(xx("Auto wall"), RAGEBOT_PATH(m_autowall));
		changed |= hell::checkbox(xx("Force shoot"), RAGEBOT_PATH(m_force_shoot), [&] {
			changed |= hell::slider_int(
				xx("Air hit chance"),
				tabs::aimbot::get_air_hitchance_holder_id(current_weapon),
				1,
				100,
				0,
				false,
				xx("%d%%")
			);
			});
		
		auto& selected_hitboxes = GET_VAR(std::vector<std::vector<int>>, RAGEBOT_PATH(m_selected_hitboxes)).at(current_weapon);
		auto& multipoint_hitboxes = GET_VAR(std::vector<std::vector<int>>, RAGEBOT_PATH(m_multipoint_hitboxes)).at(current_weapon);

		std::string hitbox_preview = get_preview_from_enums(selected_hitboxes);
		if (hell::begin_combo(xx("Hitboxes"), hitbox_preview.c_str())) {
			for (int i = 0; i < e_ragebot_hitboxes::rage_hitbox_max; ++i) {
				auto it = std::find(selected_hitboxes.begin(), selected_hitboxes.end(), i);
				bool is_selected = (it != selected_hitboxes.end());

				if (hell::selectable(hitbox_names[i], is_selected)) {
					if (is_selected) {
						selected_hitboxes.erase(it);
						auto mp_it = std::find(multipoint_hitboxes.begin(), multipoint_hitboxes.end(), i);
						if (mp_it != multipoint_hitboxes.end()) {
							multipoint_hitboxes.erase(mp_it);
						}
					}
					else {
						selected_hitboxes.push_back(i);
						std::sort(selected_hitboxes.begin(), selected_hitboxes.end());
					}
					changed = true;
				}
			}
			hell::end_combo();
		}

		std::string multipoint_preview = get_preview_from_enums(multipoint_hitboxes);
		if (hell::begin_combo(xx("Multipoint hitboxes"), multipoint_preview.c_str())) {
			if (selected_hitboxes.empty()) {
				hell::label(xx("Select hitboxes first"), { 255, 255, 255, 200 });
			}
			else {
				for (const auto& hitbox_enum : selected_hitboxes) {
					auto it = std::find(multipoint_hitboxes.begin(), multipoint_hitboxes.end(), hitbox_enum);
					bool is_selected = (it != multipoint_hitboxes.end());

					if (hell::selectable(hitbox_names[hitbox_enum], is_selected)) {
						if (is_selected) {
							multipoint_hitboxes.erase(it);
						}
						else {
							multipoint_hitboxes.push_back(hitbox_enum);
							std::sort(multipoint_hitboxes.begin(), multipoint_hitboxes.end());
						}
						changed = true;
					}
				}
			}
			hell::end_combo();
		}
		});

	end_config_change_scope(changed);
}
