#pragma once

#include <includes.h>

class c_animation {
public:
    struct animation_state_t {
        float m_fraction = 0.f;
        float m_target_fraction = 0.f;
        float m_min = 0.f;
        float m_max = 1.f;
        float m_speed = 5.f;

        float get_current_val( ) const {
            return m_min + ( m_max - m_min ) * m_fraction;
        }

        void update( float delta_time ) {
            m_fraction = ImLerp( m_fraction, m_target_fraction, m_speed * delta_time );
        }
    };

    struct animation_state_color_t {
        ImVec4 m_current = std::numeric_limits<ImVec4>::quiet_NaN( );
        float m_speed = 5.f;

        void update( const ImVec4& target, float delta_time ) {
            m_current = ImLerp( m_current, target, ImClamp( m_speed * delta_time, 0.f, 1.f ) );
        }

        ImVec4 get_current_val( ) const { return m_current; }

        animation_state_color_t( ) = default;
    };
private:
    std::unordered_map<ImGuiID, animation_state_t> m_animation_states;
    std::unordered_map<ImGuiID, animation_state_color_t> m_animation_states_color;
    float m_delta_time = 0.006632f; // this is what global vars frame time is most of the time.
public:
    animation_state_t handle_anim( ImGuiID widget_id, bool show, float min, float max, float speed ) {
        animation_state_t& anim = m_animation_states[ widget_id ];
        float target_val = show ? max : min;

        if ( anim.m_max == 0.f && anim.m_min == 0.f && anim.m_speed == 0.f ) {
            anim.m_min = min;
            anim.m_max = max;
            anim.m_speed = speed;
            anim.m_fraction = ( target_val - min ) / ( max - min );
        }

        anim.m_target_fraction = ( target_val - min ) / ( max - min );
        anim.m_speed = speed;
        anim.m_min = min;
        anim.m_max = max;

        anim.update( m_delta_time );

        return anim;
    }

    ImVec4 animate_color( ImGuiID id, const ImVec4& target, float speed ) {
        auto& anim = m_animation_states_color[ id ];
        anim.m_speed = speed;

       /* if ( ImLengthSqr( anim.m_current ) < 0.0001f )
            anim.m_current = target;*/

        anim.update( target, m_delta_time );
        return anim.get_current_val( );
    }

    animation_state_t get_anim( ImGuiID widget_id ) {
        return m_animation_states[ widget_id ];
    }

    animation_state_color_t get_anim_color( ImGuiID widget_id ) {
        return m_animation_states_color[ widget_id ];
    }
};
inline auto g_animation = std::make_unique<c_animation>( );
