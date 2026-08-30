#include "color_picker.h"
#include <cheat/menu/animation/anim.h>
#include <cheat/menu/hell_gui/colors.h>
#include <cheat/menu/hell_gui/hell_gui.h>
#include <cheat/config/config_system.h>
#include "checkbox.h"
#include "slider.h"
#include "button.h"
#include <string>
#include <cstdlib>

using namespace ImGui;

static void ColorEditRestoreHS(const float* col, float* H, float* S, float* V) {
	ImGuiContext& g = *GImGui;
	IM_ASSERT(g.ColorEditCurrentID != 0);
	if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ColorConvertFloat4ToU32(hellvec4(col[0], col[1], col[2], 0)))
		return;

	// When S == 0, H is undefined.
	// When H == 1 it wraps around to 0.
	if (*S == 0.0f || (*H == 0.0f && g.ColorEditSavedHue == 1))
		*H = g.ColorEditSavedHue;

	// When V == 0, S is undefined.
	if (*V == 0.0f)
		*S = g.ColorEditSavedSat;
}

bool ColorButton(const char* szDescId, const hellvec4& imCol, ImGuiColorEditFlags imColorEditFlags, const hellvec2& imSize) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiID id = window->GetID(szDescId);
	const float default_size = GetFrameHeight();
	const hellvec2 size(imSize.x == 0.0f ? default_size : imSize.x, imSize.y == 0.0f ? default_size : imSize.y);
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
	if (!ItemAdd(bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	if (imColorEditFlags & ImGuiColorEditFlags_NoAlpha)
		imColorEditFlags &= ~(ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf);

	hellvec4 col_rgb = imCol;
	if (imColorEditFlags & ImGuiColorEditFlags_InputHSV)
		ColorConvertHSVtoRGB(col_rgb.x, col_rgb.y, col_rgb.z, col_rgb.x, col_rgb.y, col_rgb.z);

	float grid_step = ImMin(size.x, size.y) / 2.99f;
	float rounding = 2;
	ImRect bb_inner = bb;
	float off = 0.0f;
	if ((imColorEditFlags & ImGuiColorEditFlags_NoBorder) == 0) {
		off = -0.75f; // The border (using Col_FrameBg) tends to look off when color is near-opaque and rounding is enabled. This offset seemed like a good middle ground to reduce those artifacts.
		bb_inner.Expand(off);
	}

	RenderColorRectWithAlphaCheckerboard(window->DrawList, bb_inner.Min, bb_inner.Max, GetColorU32(imCol), grid_step, hellvec2(off, off), rounding);
	window->DrawList->AddRectFilled(bb_inner.Min, bb_inner.Max, GetColorU32(imCol), rounding);

	RenderNavCursor(bb, id);

	return pressed;
}

static void RenderArrowsForVerticalBar(ImDrawList* draw_list, hellvec2 pos, hellvec2 half_sz, float bar_w, float alpha) {
	ImU32 alpha8 = IM_F32_TO_INT8_SAT(alpha);
	RenderArrowPointingAt(draw_list, hellvec2(pos.x + half_sz.x + 1, pos.y), hellvec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Right, IM_COL32(0, 0, 0, alpha8));
	RenderArrowPointingAt(draw_list, hellvec2(pos.x + half_sz.x, pos.y), half_sz, ImGuiDir_Right, IM_COL32(255, 255, 255, alpha8));
	RenderArrowPointingAt(draw_list, hellvec2(pos.x + bar_w - half_sz.x - 1, pos.y), hellvec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Left, IM_COL32(0, 0, 0, alpha8));
	RenderArrowPointingAt(draw_list, hellvec2(pos.x + bar_w - half_sz.x, pos.y), half_sz, ImGuiDir_Left, IM_COL32(255, 255, 255, alpha8));
}

static bool color_button(const char* desc_id, hellcolor col, hellvec2 position, hellvec2 size_arg = { }) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiID id = window->GetID(desc_id);

	const float default_size = GetFrameHeight() - 2;
	hellvec2 size = {
		size_arg.x == 0.0f ? default_size : size_arg.x,
		size_arg.y == 0.0f ? default_size : size_arg.y
	};

	const ImRect bb(position, position + size);
	//ItemSize( bb );
	if (!ItemAdd(bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	hellvec2 center = (bb.Min + bb.Max) * 0.5f;
	float radius = ImMin(size.x, size.y) * 0.3f;

	window->DrawList->AddCircleFilled(center, radius, col, 32);
	window->DrawList->AddCircle(center, radius + 1.f, hellcolor(80, 80, 80, 200), 32);

	return pressed;
}

bool hell::color_picker(const char* id, hellcolor& color, int pickers_inline, bool checkbox_in_line, bool alpha_bar) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	PushID(id);

	const float picker_size = GetFrameHeight();
	const float checkbox_width = hell::calc_checkbox_size();
	const float checkbox_offset = checkbox_in_line
		? checkbox_width + style.FramePadding.x
		: 0.0f;

	const float total_button_width = (picker_size - style.FramePadding.x) * pickers_inline;
	const float inline_offset = GetContentRegionAvail().x - checkbox_offset - total_button_width;

	const hellvec2 button_pos = { window->DC.CursorPos.x + inline_offset, window->DC.CursorPos.y - 1.f };

	hellvec4& col = color.Value;
	bool value_changed = false;

	if (color_button(xx("##ColorButton"), color, button_pos, { picker_size, picker_size })) {
		g.ColorPickerRef = col;
		OpenPopup(xx("picker"));
		SetNextWindowPos(g.LastItemData.Rect.GetBL() + hellvec2(0.0f, style.ItemSpacing.y + 4.f));
	}

	SetNextWindowBgAlpha(1.0f);
	PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	PushStyleVar(ImGuiStyleVar_WindowPadding, hellvec2(12.f, 12.f));
	PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
	PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
	PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
	PushStyleVar(ImGuiStyleVar_ItemSpacing, hellvec2(8.f, 8.f));
	PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, hellvec2(6.f, 4.f));

	PushStyleColor(ImGuiCol_WindowBg, hellcolor(28, 28, 28, 255).Value);
	PushStyleColor(ImGuiCol_Border, hellcolor(55, 55, 55, 255).Value);
	PushStyleColor(ImGuiCol_FrameBg, hellcolor(42, 42, 42, 255).Value);
	PushStyleColor(ImGuiCol_FrameBgHovered, hellcolor(52, 52, 52, 255).Value);
	PushStyleColor(ImGuiCol_FrameBgActive, hellcolor(60, 60, 60, 255).Value);
	PushStyleColor(ImGuiCol_Text, hellcolor(235, 235, 235, 255).Value);
	PushStyleColor(ImGuiCol_TextDisabled, hellcolor(130, 130, 130, 255).Value);
	PushStyleColor(ImGuiCol_Separator, hellcolor(55, 55, 55, 255).Value);

	if (BeginPopup(xx("picker"), ImGuiWindowFlags_NoMove)) {
		hellvec4 temp = col;

		// header: color name
		std::string title = id;
		size_t cut = title.find(xx("##"));
		if (cut != std::string::npos)
			title = title.substr(0, cut);

		// title is a real layout item, so the popup auto-fits long names
		TextUnformatted(title.c_str());

		Separator();
		Dummy(hellvec2(0.f, 2.f));

		// just the SV box + hue bar; no inline value inputs/side preview
		ImGuiColorEditFlags flags = ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview;

		SetNextItemWidth(190.f);
		if (ColorPicker4("##picker", (float*)&temp, flags)) {
			col = temp;
			value_changed = true;
		}

		if (alpha_bar) {
			Dummy(hellvec2(0.f, 4.f));
			int alpha255 = (int)(temp.w * 255.f + 0.5f);
			const float saved_width = hell::config::m_widget_width;
			hell::config::m_widget_width = 80.f;
			if (hell::slider_int_passthrough(xx("Alpha"), &alpha255, 0, 255)) {
				temp.w = ImClamp(alpha255 / 255.f, 0.f, 1.f);
				col = temp;
				value_changed = true;
			}
			hell::config::m_widget_width = saved_width;
		}

		// copy / paste color via clipboard (hex)
		Dummy(hellvec2(0.f, 4.f));
		hell::button_data_t cp_btn;
		cp_btn.m_bg_normal = hellcolor(42, 42, 42, 255);
		cp_btn.m_bg_hovered = hellcolor(52, 52, 52, 255);
		cp_btn.m_bg_pressed = hellcolor(60, 60, 60, 255);
		cp_btn.m_fg_normal = hellcolor(200, 200, 200, 255);
		cp_btn.m_fg_hovered = hellcolor(235, 235, 235, 255);
		cp_btn.m_fg_pressed = hellcolor(255, 255, 255, 255);
		cp_btn.m_frame_rounding = 4.f;

		float cp_avail = GetContentRegionAvail().x;
		float cp_w = (cp_avail - style.ItemSpacing.x) * 0.5f;

		if (hell::button(xx("Copy"), { cp_w, 0.f }, cp_btn)) {
			char buf[16];
			sprintf_s(buf, sizeof(buf), xx("%02X%02X%02X%02X"),
				(int)(temp.x * 255.f + 0.5f), (int)(temp.y * 255.f + 0.5f),
				(int)(temp.z * 255.f + 0.5f), (int)(temp.w * 255.f + 0.5f));
			SetClipboardText(buf);
		}
		hell::continue_line();
		if (hell::button(xx("Paste"), { cp_w, 0.f }, cp_btn)) {
			const char* clip = GetClipboardText();
			if (clip) {
				std::string s = clip;
				if (!s.empty() && s[0] == '#')
					s.erase(0, 1);
				if (s.size() >= 6) {
					auto hex2 = [](const std::string& str, int i) -> int {
						return (int)strtoul(str.substr(i, 2).c_str(), nullptr, 16);
					};
					temp.x = hex2(s, 0) / 255.f;
					temp.y = hex2(s, 2) / 255.f;
					temp.z = hex2(s, 4) / 255.f;
					temp.w = (s.size() >= 8) ? hex2(s, 6) / 255.f : 1.f;
					col = temp;
					value_changed = true;
				}
			}
		}
		EndPopup();
	}

	PopStyleColor(8);
	PopStyleVar(7);

	PopID();
	return value_changed;
}