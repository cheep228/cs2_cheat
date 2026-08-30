#include <format>   // PORT: std::vformat / std::make_format_args
#include "keybind.h"
#include "button.h"
#include "continue_line.h"
#include "label.h"
#include <utils/fonts/font_manager.h>
#include <utils/fonts/compressed_fonts/font_awesome.h>
#include <cheat/menu/animation/anim.h>
#include <cheat/menu/hell_gui/colors.h>
#include "slider.h"

using namespace ImGui;

const char* const s_key_names[] = {
	xx("None"),
	xx("M1"),
	xx("M2"),
	xx("Esc"),
	xx("M3"),
	xx("M4"),
	xx("M5"),
	xx("None"),
	xx("Back"),
	xx("Tab"),
	xx("None"),
	xx("None"),
	xx("BSpace"),
	xx("Enter"),
	xx("None"),
	xx("None"),
	xx("Shift"),
	xx("Ctrl"),
	xx("Alt"),
	xx("PB"),
	xx("CL"),
	xx("VK_KANA"),
	xx("Unknown"),
	xx("VK_JUNJA"),
	xx("VK_FINAL"),
	xx("VK_KANJI"),
	xx("Unknown"),
	xx("Esc"),
	xx("VK_CONVERT"),
	xx("VK_NONCONVERT"),
	xx("VK_ACCEPT"),
	xx("VK_MODECHANGE"),
	xx("Space"),
	xx("Page Up"),
	xx("Page Down"),
	xx("End"),
	xx("Home"),
	xx("Left"),
	xx("Up"),
	xx("Right"),
	xx("Down"),
	xx("VK_SELECT"),
	xx("VK_PRINT"),
	xx("VK_EXECUTE"),
	xx("Print Screen"),
	xx("Ins"),
	xx("Del"),
	xx("VK_HELP"),
	xx("0"),
	xx("1"),
	xx("2"),
	xx("3"),
	xx("4"),
	xx("5"),
	xx("6"),
	xx("7"),
	xx("8"),
	xx("9"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("A"),
	xx("B"),
	xx("C"),
	xx("D"),
	xx("E"),
	xx("F"),
	xx("G"),
	xx("H"),
	xx("I"),
	xx("J"),
	xx("K"),
	xx("L"),
	xx("M"),
	xx("N"),
	xx("O"),
	xx("P"),
	xx("Q"),
	xx("R"),
	xx("S"),
	xx("T"),
	xx("U"),
	xx("V"),
	xx("W"),
	xx("X"),
	xx("Y"),
	xx("Z"),
	xx("Left Windows"),
	xx("Right Windows"),
	xx("VK_APPS"),
	xx("Unknown"),
	xx("VK_SLEEP"),
	xx("NUMPAD0"),
	xx("NUMPAD1"),
	xx("NUMPAD2"),
	xx("NUMPAD3"),
	xx("NUMPAD4"),
	xx("NUMPAD5"),
	xx("NUMPAD6"),
	xx("NUMPAD7"),
	xx("NUMPAD8"),
	xx("NUMPAD9"),
	xx("Multiply"),
	xx("+"),
	xx("Separator"),
	xx("Subtract"),
	xx("-"),
	xx("/"),
	xx("F1"),
	xx("F2"),
	xx("F3"),
	xx("F4"),
	xx("F5"),
	xx("F6"),
	xx("F7"),
	xx("F8"),
	xx("F9"),
	xx("F10"),
	xx("F11"),
	xx("F12"),
	xx("F13"),
	xx("F14"),
	xx("F15"),
	xx("F16"),
	xx("F17"),
	xx("F18"),
	xx("F19"),
	xx("F20"),
	xx("F21"),
	xx("F22"),
	xx("F23"),
	xx("F24"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Num Lock"),
	xx("Scroll lock"),
	xx("VK_OEM_NEC_EQUAL"),
	xx("VK_OEM_FJ_MASSHOU"),
	xx("VK_OEM_FJ_TOUROKU"),
	xx("VK_OEM_FJ_LOYA"),
	xx("VK_OEM_FJ_ROYA"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("Unknown"),
	xx("LShift"),
	xx("RShift"),
	xx("LCtrl"),
	xx("RCtrl"),
	xx("LAlt"),
	xx("RAlt")
};

ImGuiKey VirtualKeyToImGuiKey(int vk) {
	switch (vk) {
	case VK_TAB: return ImGuiKey_Tab;
	case VK_LEFT: return ImGuiKey_LeftArrow;
	case VK_RIGHT: return ImGuiKey_RightArrow;
	case VK_UP: return ImGuiKey_UpArrow;
	case VK_DOWN: return ImGuiKey_DownArrow;
	case VK_PRIOR: return ImGuiKey_PageUp;
	case VK_NEXT: return ImGuiKey_PageDown;
	case VK_HOME: return ImGuiKey_Home;
	case VK_END: return ImGuiKey_End;
	case VK_INSERT: return ImGuiKey_Insert;
	case VK_DELETE: return ImGuiKey_Delete;
	case VK_BACK: return ImGuiKey_Backspace;
	case VK_SPACE: return ImGuiKey_Space;
	case VK_RETURN: return ImGuiKey_Enter;
	case VK_ESCAPE: return ImGuiKey_Escape;
		
	case VK_SHIFT: case VK_LSHIFT: return ImGuiKey_LeftShift;
	case VK_RSHIFT: return ImGuiKey_RightShift;
	case VK_CONTROL: case VK_LCONTROL: return ImGuiKey_LeftCtrl;
	case VK_RCONTROL: return ImGuiKey_RightCtrl;
	case VK_MENU: case VK_LMENU: return ImGuiKey_LeftAlt;
	case VK_RMENU: return ImGuiKey_RightAlt;
	case 'A': return ImGuiKey_A;
	case 'B': return ImGuiKey_B;
	case 'C': return ImGuiKey_C;
	case 'D': return ImGuiKey_D;
	case 'E': return ImGuiKey_E;
	case 'F': return ImGuiKey_F;
	case 'G': return ImGuiKey_G;
	case 'H': return ImGuiKey_H;
	case 'I': return ImGuiKey_I;
	case 'J': return ImGuiKey_J;
	case 'K': return ImGuiKey_K;
	case 'L': return ImGuiKey_L;
	case 'M': return ImGuiKey_M;
	case 'N': return ImGuiKey_N;
	case 'O': return ImGuiKey_O;
	case 'P': return ImGuiKey_P;
	case 'Q': return ImGuiKey_Q;
	case 'R': return ImGuiKey_R;
	case 'S': return ImGuiKey_S;
	case 'T': return ImGuiKey_T;
	case 'U': return ImGuiKey_U;
	case 'V': return ImGuiKey_V;
	case 'W': return ImGuiKey_W;
	case 'X': return ImGuiKey_X;
	case 'Y': return ImGuiKey_Y;
	case 'Z': return ImGuiKey_Z;
	default: return ImGuiKey_NamedKey_BEGIN;
	}
}

keybind_t hell::create_keybind(ImGuiID id, fnv1a_t holder_id, e_keybind_type keybind_type) {
	keybind_t kb;

	kb.m_holder_id = holder_id;
	kb.m_keybind_type = keybind_type;

	switch (keybind_type) {
	case e_keybind_type::keybind_type_slider_int:
		kb.m_default_val = GET_VAR(int, holder_id);
		kb.m_override_val = std::get<int>(kb.m_default_val);
		break;
	case e_keybind_type::keybind_type_slider_float:
		kb.m_default_val = GET_VAR(float, holder_id);
		kb.m_override_val = std::get<float>(kb.m_default_val);
		break;
	case e_keybind_type::keybind_type_checkbox:
		kb.m_default_val = GET_VAR(bool, holder_id);
		kb.m_override_val = std::get<bool>(kb.m_default_val);
		break;
	}

	return kb;
}

bool hell::keybind(ImGuiID id, keybind_t& keybind, fnv1a_t holder_id, bool value_changed) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	SameLine();

	ImGuiContext& g = *GImGui;

	const ImGuiStyle& style = g.Style;
	ImGuiIO* io = &GetIO();


	hellvec2 label_size = CalcTextSize(s_key_names[std::max(0, keybind.m_key)]);
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + label_size);
	const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + hellvec2(window->Pos.x + window->Size.x - window->DC.CursorPos.x, label_size.y));

	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	const bool hovered = IsItemHovered();
	const bool edit_requested = hovered && io->MouseClicked[0];
	const bool style_requested = hovered && io->MouseClicked[1];

	if (edit_requested) {
		if (g.ActiveId != id) {
			memset(io->MouseDown, 0, sizeof(io->MouseDown));
			memset(io->KeysData, 0, sizeof(io->KeysData));
			keybind.m_key = 0;
		}

		SetActiveID(id, window);
		FocusWindow(window);
	}
	else if (!hovered && io->MouseClicked[0] && g.ActiveId == id)
		ClearActiveID();

	bool value_changed_ = false;
	int key = keybind.m_key;

	if (g.ActiveId == id) {
		for (auto i = 0; i < 5; i++) {
			if (IsMouseDown(i)) {
				switch (i) {
				case 0:
					key = VK_LBUTTON;
					break;
				case 1:
					key = VK_RBUTTON;
					break;
				case 2:
					key = VK_MBUTTON;
					break;
				case 3:
					key = VK_XBUTTON1;
					break;
				case 4:
					key = VK_XBUTTON2;
				}
				value_changed_ = true;
				ClearActiveID();
			}
		}

		if (!value_changed_) {
			for (auto i = VK_BACK; i <= VK_RMENU; i++) {
				ImGuiKey key2 = VirtualKeyToImGuiKey(i);
				if (!IsNamedKey(key2))
					continue;
				if (IsKeyDown(key2)) {
					key = i;
					value_changed_ = true;
					ClearActiveID();
				}
			}

			switch (key) {
			case VK_LSHIFT: case VK_RSHIFT: key = VK_SHIFT; break;
			case VK_LCONTROL: case VK_RCONTROL: key = VK_CONTROL; break;
			case VK_LMENU: case VK_RMENU: key = VK_MENU; break;
			}
		}

		if (IsKeyPressed(ImGuiKey_Escape)) {
			keybind.m_key = 0;
			ClearActiveID();
		}
		else
			keybind.m_key = key;
	}

	const bool bLeftClicked = hovered && io->MouseClicked[1];

	if (bLeftClicked && !(g.ActiveId == id) && keybind.m_key != 0) {
		OpenPopup(std::to_string(id).c_str());
	}

	PushStyleVar(ImGuiStyleVar_WindowPadding, hellvec2(10, 10));
	PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);

	PushStyleColor(ImGuiCol_PopupBg, hellcolor(35, 35, 35, 220).Value);
	PushStyleColor(ImGuiCol_Border, hellcolor(255, 255, 255, 25).Value);

	SetNextWindowSize({ 200, -1 });
	if (BeginPopup(std::to_string(id).c_str())) {
		hell::label(xx("Type"), hell::colors::m_text_unhovered);

		hell::continue_line();

		hell::button_data_t button_data_normal;
		button_data_normal.m_animation_speed = 15.f;
		hell::button_data_t button_data_selected;
		button_data_selected.m_animation_speed = 15.f;
		hellcolor accent_color = hellcolor(140, 90, 190, 255);
		button_data_selected.m_bg_normal = hellcolor(140, 90, 190, 178).Value; // 0.7 * 255
		button_data_selected.m_bg_hovered = hellcolor(140, 90, 190, 153).Value; // 0.6 * 255
		button_data_selected.m_bg_pressed = hellcolor(140, 90, 190, 128).Value; // 0.5 * 255
		button_data_selected.m_fg_normal = { 255, 255, 255 };
		button_data_selected.m_fg_hovered = { 255, 255, 255 };
		button_data_selected.m_fg_pressed = { 255, 255, 255 };

		if (hell::button(xx("Hold"), { 0, 14.f }, (keybind.m_mode == e_key_mode::key_mode_hold) ? button_data_selected : button_data_normal))
			keybind.m_mode = e_key_mode::key_mode_hold;

		hell::continue_line();

		if (hell::button(xx("Toggle"), { 0, 14.f }, (keybind.m_mode == e_key_mode::key_mode_toggle) ? button_data_selected : button_data_normal))
			keybind.m_mode = e_key_mode::key_mode_toggle;

		if (keybind.m_keybind_type == e_keybind_type::keybind_type_checkbox) {
			Dummy({ 0.f, 5.f });
			hell::label(xx("Mode"), hell::colors::m_text_unhovered);
			hell::continue_line();

			if (hell::button(xx("On"), { 0, 14.f }, (keybind.m_is_on_mode) ? button_data_selected : button_data_normal)) {
				keybind.m_is_on_mode = true;
			}

			hell::continue_line();

			if (hell::button(xx("Off"), { 0, 14.f }, (!keybind.m_is_on_mode) ? button_data_selected : button_data_normal)) {
				keybind.m_is_on_mode = false;
			}
		}

		if (keybind.m_keybind_type == e_keybind_type::keybind_type_slider_int) {
			if (!std::holds_alternative<int>(keybind.m_override_val)) {
				if (std::holds_alternative<int>(keybind.m_default_val))
					keybind.m_override_val = std::get<int>(keybind.m_default_val);
				else
					keybind.m_override_val = 0;
			}
			auto min_max = hell::_registered_internal::get_registered(holder_id);
			hell::slider_int_passthrough(xx("Value"), reinterpret_cast<int*>(&keybind.m_override_val), min_max.first, min_max.second, id);
		}
		else if (keybind.m_keybind_type == e_keybind_type::keybind_type_slider_float) {
			if (!std::holds_alternative<float>(keybind.m_override_val)) {
				if (std::holds_alternative<float>(keybind.m_default_val))
					keybind.m_override_val = std::get<float>(keybind.m_default_val);
				else
					keybind.m_override_val = 0.f;
			}
			auto min_max = hell::_registered_internal::get_registered(holder_id);
			hell::slider_float_passthrough(xx("Value"), reinterpret_cast<float*>(&keybind.m_override_val), min_max.first, min_max.second, id);
		}

		EndPopup();
	}

	// ✅ FIX: Always pop style vars/colors regardless of BeginPopup result.
	// BeginPopup returns false while the popup is closed/animating, but Push*
	// calls are unconditional above, so Pop* must also be unconditional.
	PopStyleColor(2);
	PopStyleVar(3);

	char buf_display[64] = xx("-");
	auto key_str = (keybind.m_key != -1) ? s_key_names[keybind.m_key] : buf_display;
	std::string active = std::vformat(xx("{}"), std::make_format_args(key_str));

	strcpy_s(buf_display, active.c_str());

	hellvec2 cursor_screen = GetCursorScreenPos();
	hellvec2 text_size = CalcTextSize(buf_display);

	hellvec2 text_pos = hellvec2(cursor_screen.x + GetContentRegionAvail().x - text_size.x - GetStyle().FramePadding.x, frame_bb.Min.y);

	ImRect bg_rect = { text_pos, text_pos + text_size };
	bg_rect.Expand({ 5.f, 3.f });
	window->DrawList->AddRectFilled(bg_rect.Min, bg_rect.Max, hellcolor(30, 30, 30, 200), 3.f);

	window->DrawList->AddText(text_pos, g.ActiveId == id ? hellcolor(140, 90, 190, 255) : hellcolor(255, 255, 255, 120), buf_display);

	return value_changed_;
}

struct keybind_state_t {
	float m_height = 30.f;
};
std::unordered_map<ImGuiID, keybind_state_t> s_keybind_states;

void hell::keybind_handler(const std::string& id_string, hellvec2 open_pos, fnv1a_t holder_id, e_keybind_type keybind_type, bool value_changed) {
	std::string id_string_popup = id_string + xx("##popup_kb");
	const char* popup_id = id_string_popup.c_str();

	// Stable map key: use holder_id directly instead of hashing the label string.
	// holder_id is unique per config variable and deterministic across sessions,
	// keeping save/load/UI consistent. Using label hash caused conflicts when
	// multiple widgets had the same label text (e.g., multiple "Enabled" checkboxes).
	ImGuiID id = static_cast<ImGuiID>(holder_id);

	std::variant<bool*, int*, float*> val_ptr;

	switch (keybind_type) {
	case e_keybind_type::keybind_type_checkbox:
		val_ptr = &GET_VAR(bool, holder_id);
		break;
	case e_keybind_type::keybind_type_slider_int:
		val_ptr = &GET_VAR(int, holder_id);
		break;
	case e_keybind_type::keybind_type_slider_float:
		val_ptr = &GET_VAR(float, holder_id);
		break;
	}

	keybind_state_t& state = s_keybind_states[id];

	if (IsItemHovered() && IsMouseReleased(ImGuiMouseButton_Right)) {
		OpenPopup(popup_id);
	}

	if (IsPopupOpen(popup_id)) {
		hellvec2 padding = { 10.f, 10.f };
		hellvec2 adjusted_size = { 150.f, state.m_height };
		hellvec2 button_size = { 15.f, 15.f };

		SetNextWindowPos(open_pos);
		SetNextWindowSize(adjusted_size);

		PushStyleColor(ImGuiCol_PopupBg, hellcolor(40, 40, 40, 255).Value);
		PushStyleColor(ImGuiCol_Border, hellcolor(48, 48, 48, 255).Value);
		PushStyleVar(ImGuiStyleVar_PopupRounding, 5.0f);
		PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
		PushStyleVar(ImGuiStyleVar_WindowPadding, padding);

		if (BeginPopup(popup_id, ImGuiWindowFlags_NoScrollbar)) {
			BeginGroup();
			{
				const char* text = xx("Bindings");
				hellvec2 text_size = CalcTextSize(text);

				hellvec2 text_pos = { open_pos.x + adjusted_size.x * 0.5f - text_size.x * 0.5f, open_pos.y + padding.y };
				GetWindowDrawList()->AddText(text_pos, GetColorU32(ImGuiCol_Text), text);

				Dummy({ 0.f, text_size.y + 7.f });

				Separator();

				Dummy({ 0.f, 7.f });

				auto& widget_keybinds = GET_VAR(kb_map_t, CONFIG_PATH(m_widget_keybinds));

				BeginGroup();
				{
					int index_to_erase = -1;
					for (int i = 0; i < widget_keybinds[id].size(); i++) {
						hell::button_data_t btn_data_minus{};
						btn_data_minus.m_bg_hovered = { 0, 0, 0, 0 };
						btn_data_minus.m_bg_normal = { 0, 0, 0, 0 };
						btn_data_minus.m_bg_pressed = { 0, 0, 0, 0 };
						btn_data_minus.m_fg_hovered = { 255, 75, 75 };

						PushID(i + GetID(xx("##kb_hdnler_button")));
						PushFont(g_font_manager->m_fa_small);
						if (hell::button(ICON_FA_TRASH, { 15.f, 10.f }, btn_data_minus)) {
							index_to_erase = i;
						}
						PopFont();
						PopID();

						hell::continue_line();

						hell::keybind(i + GetID(xx("##kb_hdnler_keybind")), widget_keybinds[id][i], holder_id, value_changed);

						Dummy({ 0.f, 2.f });
					}
					if (index_to_erase >= 0) {
						widget_keybinds[id].erase(widget_keybinds[id].begin() + index_to_erase);
					}

					if (!widget_keybinds[id].empty()) {
						Separator();

						Dummy({ 0.f, 5.f });
					}

					{ 
						if (hell::button(xx("Add Keybind"), { -1.f, 0.f })) {
							widget_keybinds[id].emplace_back(create_keybind(id, holder_id, keybind_type));
						}
					}
				}
				EndGroup();
			}
			EndGroup();

			state.m_height = GetItemRectSize().y + (padding.y * 2);

			EndPopup();
		}

	
		PopStyleVar(3);
		PopStyleColor(2);
	}
}

void hell::render_keybind_list(const kb_map_t& widget_keybinds) {

}