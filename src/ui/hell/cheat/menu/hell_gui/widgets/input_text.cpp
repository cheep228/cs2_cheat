#include "input_text.h"
#include <cheat/menu/hell_gui/colors.h>
#include <cheat/menu/hell_gui/hell_gui.h>

using namespace ImGui;

bool hell::input_text(const char* label, char* buf, size_t buf_size, const char* placeholder, ImGuiInputTextFlags flags, float width, int custom_id) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label) + custom_id;

	hellvec2 label_size(0.0f, 0.0f);
	bool draw_label = label && label[0] != '\0';
	if (draw_label) {
		label_size = CalcTextSize(label, NULL, true);
		if (label_size.x == 0.0f) {
			draw_label = false;
		}
	}

	float input_width = width;
	if (input_width < 0.f) {
		if (draw_label) {
			input_width = GetContentRegionAvail().x - label_size.x - style.ItemInnerSpacing.x;
		}
		else {
			input_width = GetContentRegionAvail().x;
		}
		input_width = ImMax(1.0f, input_width);
	}

	const float frame_height = GetFrameHeight() + 2.f;
	hellvec2 input_pos = window->DC.CursorPos;
	if (draw_label) {
		input_pos.x = window->WorkRect.Max.x - input_width;
	}

	ImRect frame_bb(input_pos, input_pos + hellvec2(input_width, frame_height));

	hellcolor bg_color = { 45, 45, 45, 120 };
	constexpr float bg_rect_radius = 2.f;

	float* bg_alpha = GetStateStorage()->GetFloatRef(id + 1, 0.47f);
	bg_color.Value.w = *bg_alpha;

	window->DrawList->AddRectFilled(frame_bb.Min, frame_bb.Max, bg_color, bg_rect_radius);
	window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, hellcolor(80, 80, 80, 50), bg_rect_radius);

	PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
	PushStyleColor(ImGuiCol_Text, hell::colors::m_text_selected.Value);
	PushStyleColor(ImGuiCol_TextDisabled, hell::colors::m_text_unhovered.Value);
	PushStyleVar(ImGuiStyleVar_FramePadding, { style.FramePadding.x, style.FramePadding.y + 2.f });
	PushStyleVar(ImGuiStyleVar_FrameRounding, bg_rect_radius);

	SetNextItemWidth(input_width);

	std::string input_id_str = std::string(label) + "##input_" + std::to_string(id);
	bool value_changed = InputText(input_id_str.c_str(), buf, buf_size, flags);

	PopStyleVar(2);
	PopStyleColor(3);

	const bool item_active = IsItemActive();
	const bool item_focused = IsItemFocused();
	bool hovered = IsItemHovered();

	float target_bg_alpha = item_active || item_focused ? 1.f : hovered ? 0.6f : 0.47f;
	*bg_alpha = ImLerp(*bg_alpha, target_bg_alpha, g.IO.DeltaTime * 8.f);

	if (draw_label) {
		float* label_alpha = GetStateStorage()->GetFloatRef(id + 2, 0.f);
		float target_label_alpha = item_active || item_focused ? 1.f : hell::colors::m_text_unhovered.Value.w;
		*label_alpha = ImLerp(*label_alpha, target_label_alpha, g.IO.DeltaTime * 8.f);

		hellvec2 label_pos = hellvec2(
			window->DC.CursorPos.x,
			frame_bb.Min.y + ((frame_bb.Max.y - frame_bb.Min.y) - label_size.y) * 0.5f
		);
		ImVec4 label_color = ImVec4(hell::colors::m_text_selected.Value.x, hell::colors::m_text_selected.Value.y, hell::colors::m_text_selected.Value.z, *label_alpha);
		window->DrawList->AddText(label_pos, ColorConvertFloat4ToU32(label_color), label);
	}

	if (placeholder && buf[0] == '\0' && !item_focused) {
		hellvec2 placeholder_pos = frame_bb.Min + hellvec2(style.FramePadding.x + 4.f, style.FramePadding.y + 2.f);
		window->DrawList->AddText(placeholder_pos, hell::colors::m_text_unhovered, placeholder);
	}

	return value_changed;
}