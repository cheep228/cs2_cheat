#pragma once

#include <context.h>
#include <sdk/datatypes/fnv1a.h>

namespace hell {
    struct SliderObj_t {
        // State: General
        float               flFrameRounding = 2.f;
        float                    flWidthAdd = 0.f;
        float                    flWidthSub = 0.f/*300.f*/;

        ImColor           imBackgroundColor = ImColor( 40, 40, 40, 255 );

        bool                     bDrawDebug = false;

        float         flAnimationTime_Value = 10.f; // 0 = slow

        // State: Hover
        float        flAnimationTime_Hover = 10.f; // 0 = slow

        // State: Active
        float        flAnimationTime_Active = 25.f; // 0 = slow
    };

    namespace _registered_internal {
        inline std::unordered_map<fnv1a_t, std::pair<float, float>> m_registered_min_max;

        inline void register_min_max( fnv1a_t id, float min, float max ) {
            m_registered_min_max[ id ] = { min, max };
        }

        inline std::pair<float, float> get_registered( fnv1a_t id ) {
            return m_registered_min_max[ id ];
        }
    }

    bool slider_int( const char* label, fnv1a_t holder_id, int min, int max, int custom_id = 0, bool force_custom_id = false, const char* format = "%d", ImGuiSliderFlags flags = 0 );
    bool slider_int_passthrough( const char* label, int* v, int min, int max, int custom_id = 0, bool force_custom_id = false, const char* format = "%d", ImGuiSliderFlags flags = 0 );
    bool slider_float( const char* label, fnv1a_t holder_id, float min_, float max_, int custom_id = 0, bool force_custom_id = false, const char* format = "%d", ImGuiSliderFlags flags = 0 );
    bool slider_float_passthrough( const char* label, float* v, float min, float max, int custom_id = 0, bool force_custom_id = false, const char* format = "%d", ImGuiSliderFlags flags = 0 );
    
    bool slider_none( const char* label, fnv1a_t holder_id, int min, int max, int custom_id = 0, const char* format = "%d", ImGuiSliderFlags flags = 0 );
    bool min_dmg_slider( const char* label, fnv1a_t holder_id, int min, int max, int custom_id = 0, const char* format = "%d", ImGuiSliderFlags flags = 0 );

    bool slider_scalar( const char* label, ImGuiDataType data_type, std::variant<fnv1a_t, void*> v, const void* min, const void* max, const char* format, ImGuiSliderFlags flags, int custom_id, bool force_custom_id, bool passthrough );
}