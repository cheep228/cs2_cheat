#include "child.h"
#include "label.h"
#include <utils/fonts/font_manager.h>

using namespace ImGui;

void hell::child(const char* str_id, hellvec2 size, std::function<void()> content, bool draw_label) {
    auto* window = GetCurrentWindow();
    hellvec2 cursor_pos = window->DC.CursorPos;
    ImRect bg_rect = { cursor_pos, cursor_pos + size };


    GetWindowDrawList()->AddRectFilled(bg_rect.Min, bg_rect.Max, hellcolor(35, 35, 35, 150), 4.f);
    GetWindowDrawList()->AddRect(bg_rect.Min, bg_rect.Max, hellcolor(55, 55, 55, 150), 4.f);




    if (draw_label)
    {
        hellvec2 text_size = CalcTextSize(str_id);
        const float pad_x = 8.f;
        const float pad_y = 3.f;
        hellvec2 label_pos =
        {
            bg_rect.Min.x + 12.f,
            bg_rect.Min.y - (text_size.y * 0.5f) + 8.f
        };

        ImRect label_rect =
        {
            { label_pos.x - pad_x, label_pos.y - pad_y },
            { label_pos.x + text_size.x + pad_x, label_pos.y + text_size.y + pad_y }
        };

        GetWindowDrawList()->AddText(
            label_pos,
            hellcolor(220, 220, 220, 255),
            str_id
        );
    }

    BeginChild(str_id, size, ImGuiChildFlags_AlwaysUseWindowPadding); {
        if (draw_label) {
            SetCursorPosY(GetCursorPosY() + 8.f);
            Dummy({ 0.f, 0.f });
        }
        content();
    }
    EndChild();
}