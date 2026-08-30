#include "button.h"
#include <cheat/menu/animation/anim.h>

using namespace ImGui;

bool hell::button( const char* label, hellvec2 size, button_data_t data ) {
    ImGuiWindow* window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID( label ) + data.m_custom_id;
    const ImVec2 label_size = CalcTextSize( label, NULL, true );

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size_internal = CalcItemSize( size, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f );
    const ImRect bb( pos, pos + size_internal );

    ItemSize( size_internal, style.FramePadding.y );
    if ( !ItemAdd( bb, id ) )
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held, ImGuiButtonFlags_None );

    // anims
    hellcolor target_bg = ( held && hovered ) ? data.m_bg_pressed : hovered ? data.m_bg_hovered : data.m_bg_normal;
    hellcolor target_fg = ( held && hovered ) ? data.m_fg_pressed : hovered ? data.m_fg_hovered : data.m_fg_normal;

    hellcolor col_bg = g_animation->animate_color( id + 1, target_bg, data.m_animation_speed );
    hellcolor col_fg = g_animation->animate_color( id + 2, target_fg, data.m_animation_speed );

    // render
    window->DrawList->AddRectFilled( bb.Min, bb.Max, col_bg, data.m_frame_rounding );

    hellvec2 text_pos = bb.GetCenter( );
    hellvec2 text_size = GetFont( )->CalcTextSizeA(GetFont( )->LegacySize, FLT_MAX, 0.f, label );

    text_pos.y -= text_size.y * 0.5f;

    switch ( data.m_text_alignment ) {
    case e_button_alignment::button_alignment_center:
        text_pos.x -= text_size.x * 0.5f;
        break;
    case e_button_alignment::button_alignment_left:
        text_pos.x = bb.Min.x + GetStyle( ).FramePadding.x;
        text_pos.x = ImClamp<float>( text_pos.x, bb.Min.x, bb.Max.x - text_size.x );
        break;
    case e_button_alignment::button_alignment_right:
        text_pos.x = bb.Max.x - text_size.x - GetStyle( ).FramePadding.x;
        text_pos.x = ImClamp<float>( text_pos.x, bb.Min.x, bb.Max.x - text_size.x );
        break;
    }

    window->DrawList->AddText( text_pos, col_fg, label );

    return pressed;
}

bool hell::button_pos( const char* label, hellvec2 size, hellvec2 pos, button_data_t data ) {
    ImGuiWindow* window = GetCurrentWindow( );
    if ( window->SkipItems )
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID( label ) + data.m_custom_id;
    const ImVec2 label_size = CalcTextSize( label, NULL, true );

    ImVec2 size_internal = CalcItemSize( size, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f );
    const ImRect bb( pos, pos + size_internal );

    if ( !ItemAdd( bb, id ) )
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior( bb, id, &hovered, &held, ImGuiButtonFlags_None );

    // anims
    hellcolor target_bg = ( held && hovered ) ? data.m_bg_pressed : hovered ? data.m_bg_hovered : data.m_bg_normal;
    hellcolor target_fg = ( held && hovered ) ? data.m_fg_pressed : hovered ? data.m_fg_hovered : data.m_fg_normal;

    hellcolor col_bg = g_animation->animate_color( id + 1, target_bg, data.m_animation_speed );
    hellcolor col_fg = g_animation->animate_color( id + 2, target_fg, data.m_animation_speed );

    // render
    window->DrawList->AddRectFilled( bb.Min, bb.Max, col_bg, data.m_frame_rounding );

    hellvec2 text_pos = bb.GetCenter( );
    hellvec2 text_size = GetFont( )->CalcTextSizeA( GetFont( )->LegacySize, FLT_MAX, 0.f, label );

    text_pos.y -= text_size.y * 0.5f;

    switch ( data.m_text_alignment ) {
    case e_button_alignment::button_alignment_center:
        text_pos.x -= text_size.x * 0.5f;
        break;
    case e_button_alignment::button_alignment_left:
        text_pos.x = bb.Min.x + GetStyle( ).FramePadding.x;
        text_pos.x = ImClamp<float>( text_pos.x, bb.Min.x, bb.Max.x - text_size.x );
        break;
    case e_button_alignment::button_alignment_right:
        text_pos.x = bb.Max.x - text_size.x - GetStyle( ).FramePadding.x;
        text_pos.x = ImClamp<float>( text_pos.x, bb.Min.x, bb.Max.x - text_size.x );
        break;
    }

    window->DrawList->AddText( text_pos, col_fg, label );

    return pressed;
}

