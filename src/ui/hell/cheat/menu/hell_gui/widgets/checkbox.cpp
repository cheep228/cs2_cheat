#include "checkbox.h"
#include <map>
#include "button.h"
#include "label.h"
#include "continue_line.h"
#include <cheat/input.h>
#include "keybind.h"
#include <cheat/menu/animation/anim.h>
#include <cheat/menu/hell_gui/colors.h>
#include <utils/fonts/font_manager.h>

using namespace ImGui;

bool hell::checkbox_passthrough(const char* label, bool* v, int custom_id) {
    return hell::checkbox(label, v, 0, custom_id, true);
}

void draw_checkmark(ImDrawList* draw_list, ImVec2 center, ImU32 color, float size = 6.0f, float thickness = 1.5f) {
    center.x--;
    ImVec2 start(center.x - size * 0.4f, center.y);
    ImVec2 mid(center.x - size * 0.1f, center.y + size * 0.4f);
    ImVec2 end(center.x + size * 0.5f, center.y - size * 0.5f);

    draw_list->AddLine(start, mid, color, thickness);
    draw_list->AddLine(mid, end, color, thickness);
}

bool hell::checkbox(const char* label, std::variant<fnv1a_t, bool*> v, std::function<void()> content, int custom_id, bool passthrough) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    bool* val = nullptr;

    if (!passthrough) {
        if (std::holds_alternative<fnv1a_t>(v)) {
            val = &GET_VAR(bool, std::get<fnv1a_t>(v));
        }
    }
    else {
        if (std::holds_alternative<bool*>(v)) {
            val = std::get<bool*>(v);
        }
    }

    if (!val)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiID id = window->GetID(label) + custom_id;

    constexpr float animation_speed = 22.f;
    constexpr float animation_speed_cog = 35.f;

    hellvec2 cursor_pos = window->DC.CursorPos;

    float frame_height = GetFrameHeight() - 2.f;
    float checkbox_size = hell::calc_checkbox_size();
    float width = checkbox_size;
    float height = checkbox_size;
    float radius = checkbox_size * 0.25f;

    constexpr float settings_button_width = 15.f;

    float avail_width = GetContentRegionAvail().x;
    float vertical_offset = (frame_height - height) * 0.5f;

    hellvec2 checkbox_pos = { cursor_pos.x + avail_width - width, cursor_pos.y + vertical_offset };
    hellvec2 settings_button_pos = { checkbox_pos.x - settings_button_width - style.FramePadding.x, cursor_pos.y + (frame_height - settings_button_width) * 0.5f };

    hellvec2 bb_min = cursor_pos;
    hellvec2 bb_max = { cursor_pos.x + avail_width, cursor_pos.y + frame_height };
    ImRect total_bb = { bb_min, bb_max };

    if (content) {
        std::string str_id_str = std::string(label) + xx("_popup");
        const char* str_id = str_id_str.c_str();

        static hell::button_data_t btn_data;
        btn_data.m_bg_normal = {};
        btn_data.m_bg_hovered = {};
        btn_data.m_bg_pressed = {};
        btn_data.m_fg_normal = g_animation->animate_color(GetID(str_id) + 0xF, IsPopupOpen(str_id) ? hellcolor(255, 255, 255) : hellcolor(255, 255, 255, 60), animation_speed_cog);
        btn_data.m_fg_hovered = g_animation->animate_color(GetID(str_id) + 0xFF, IsPopupOpen(str_id) ? hellcolor(255, 255, 255) : hellcolor(255, 255, 255, 100), animation_speed_cog);
        btn_data.m_custom_id = GetID(str_id);
        PushFont(g_font_manager->m_fa_large);
        ImVec2 icon_size = CalcTextSize(ICON_FA_GEAR);

        hellvec2 gear_pos = settings_button_pos + hellvec2{ (settings_button_width - icon_size.x) * 0.5f, (settings_button_width - icon_size.y) * 0.5f };

        bool settings_pressed = hell::button_pos(ICON_FA_GEAR, { settings_button_width, g_font_manager->m_fa_large->LegacySize }, gear_pos, btn_data);
        PopFont();

        if (settings_pressed) {
            OpenPopup(str_id);
        }

        if (IsPopupOpen(str_id)) {
            hellvec2 padding = { 10.f, 10.f };
            SetNextWindowSize({ 300.f, 0.f });
            SetNextWindowPos({ total_bb.Min.x, total_bb.Max.y + 3.f });
            PushStyleColor(ImGuiCol_PopupBg, hellcolor(40, 40, 40, 240).Value);
            PushStyleColor(ImGuiCol_Border, hellcolor(48, 48, 48, 255).Value);
            PushStyleVar(ImGuiStyleVar_PopupRounding, 5.0f);
            PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
            PushStyleVar(ImGuiStyleVar_WindowPadding, padding);

            if (BeginPopup(str_id)) {
                content();
                EndPopup();
            }

            PopStyleVar(3);
            PopStyleColor(2);
        }
    }

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed) {
        *val = !*val;
        MarkItemEdited(id);
    }

    {
        hellcolor desired_bg_color = *val ? hellcolor(140, 90, 190, 255) : hellcolor(45, 45, 45, 180);
        hellcolor bg_color = g_animation->animate_color(id + 0xF, desired_bg_color, animation_speed);
        window->DrawList->AddRectFilled(checkbox_pos, checkbox_pos + hellvec2(width, height), bg_color, radius);
        if (!*val)
            window->DrawList->AddRect({ checkbox_pos.x - 1.f, checkbox_pos.y - 1.f }, checkbox_pos + hellvec2(width + 1.f, height + 1.f), hellcolor(80, 80, 80, 100), radius);

        {
            if (*val) {
                hellvec2 checkmark_pos = checkbox_pos + hellvec2{ width * 0.5f, height * 0.5f };
                draw_checkmark(window->DrawList, checkmark_pos, hellcolor(255, 255, 255, (int)(desired_bg_color.Value.w * 255.f)), width * 0.5f);
            }
        }
    }


    {
        hellvec2 label_size = CalcTextSize(label);
        hellvec2 label_pos = hellvec2(cursor_pos.x, cursor_pos.y + (frame_height - label_size.y) * 0.5f);

        hellcolor desired_label_color = *val ? hellcolor(255, 255, 255) : hellcolor(255, 255, 255, 120);
        hellcolor label_color = g_animation->animate_color(id + 0xFFF, desired_label_color, animation_speed);
        window->DrawList->AddText(label_pos, label_color, label);
    }

    if (!passthrough)
        hell::keybind_handler(label, total_bb.GetCenter(), std::get<fnv1a_t>(v), e_keybind_type::keybind_type_checkbox);

    return pressed;
}

float hell::calc_checkbox_size() {
    return GetFrameHeight() - 6.f;
}