#pragma once

#include <map>
#include <includes.h>
#include <cheat/features/visuals/visuals.h>
#include <cheat/menu/hell_gui/hell_gui.h>
#include "animation/anim.h"
#include <sdk/interfaces/scene_system.h>
#include <sdk/interfaces/panorama.h>

inline const char* shadow_names[ ] = {
	xx( "None" ),
	xx( "Drop" ),
	xx( "Full" )
};

inline const char* font_names[ ] = {
	xx( "Smallest Pixel" ),
	xx( "Verdana" ),
	xx( "Verdana Bold" ),
	xx( "Calibri" ),
	xx( "Tahoma" )
};

namespace visual_preview {
	inline bool m_is_dragging_esp_element = false;
	inline std::map< std::pair<c_visuals::e_element_bb_position, int/*row*/>, bool > s_occupied_positions;

	enum e_preview_type : unsigned int {
		preview_type_bar = 0,
		preview_type_text,
		preview_type_icon
	};

	class c_model_preview {
	public:
		bool capture_texture( c_scene_layer* scene_layer ) {
			if ( !scene_layer || !scene_layer->m_texture_handle || !scene_layer->m_texture_handle->m_texture )
				return false;

			m_current_model_texture = scene_layer->m_texture_handle->m_texture;
		}

		bool draw_model( ) {
			if ( !m_current_model_texture )
				return false;

			auto draw_list = hell::window::m_fg_drawlist;

			draw_list->AddImage( ( ImTextureID )m_current_model_texture->m_texture_SRV1, { 25.f, 25.f }, { 500.f, 500.f } );
		}

		__int64 get_faux_item_id( int def_index, __int64 paint_kit_id ) {
			return ( 0xF000000000000000 | paint_kit_id << 16 | def_index );
		}

		bool init( void ) {
			if ( m_created_model_preview )
				return false;

			static const char model_preview_js[ ] = R"(
					var parent = $.GetContextPanel();

					var createPlayerModelGlowPreview = function (id, labelId, playerModel, itemId) {
					var container = $.CreatePanel('Panel', parent, '', { style: 'flow-children: none;' });
					var previewPanel = $.CreatePanel('MapPlayerPreviewPanel', container, id, {
					  map: "ui/buy_menu",
					  camera: "cam_loadoutmenu_ct",
					  "require-composition-layer": true,
					  playermodel: playerModel,
					  playername: "vanity_character",
					  animgraphcharactermode: "buy-menu",
					  player: true,
					  mouse_rotate: false,
					  sync_spawn_addons: true,
					  "transparent-background": true,
					  "pin-fov": "vertical",
					  csm_split_plane0_distance_override: "250.0",
					  style: "y: 5px; vertical-align: top; width: 300px; height: 300px; horizontal-align: center;"
					});
					previewPanel.EquipPlayerWithItem(itemId);
					$.CreatePanel('Label', container, labelId, { style: 'vertical-align: top; horizontal-align: center;' });
				  };
			)";

			c_ui_engine* ui_engine = g_interfaces->m_panorama_ui_engine->get_ui_engine( );
			if ( !ui_engine )
				return false;

			c_ui_panel* wish_panel = nullptr;

			for ( int i = 0; i < ui_engine->m_panel_count; i++ ) {
				panel_data_t* panel_data = &ui_engine->m_panels_array[ i ];
				if ( !panel_data )
					continue;

				c_ui_panel* panel = panel_data->m_panel;
				if ( !panel )
					continue;

				if ( !panel->m_panel_name )
					continue;

				fnv1a_t hashed_panel_name = fnv_hash( panel->m_panel_name );

				if ( hashed_panel_name == fnv_hash( "CSGOMainMenu" ) ) {
					wish_panel = panel;
					break;
				}
			}

			if ( !wish_panel )
				return false;

			

			ui_engine->run_script( wish_panel, model_preview_js );

			ui_engine->run_script( wish_panel,
				(
					std::string( "createPlayerModelGlowPreview( 'ModelGlowPreviewPlayerTT" ) +
					"', '" +
					"ModelGlowPreviewPlayerTTLabel" +
					"', '" +
					"characters/models/ctm_sas/ctm_sas.vmdl" +
					"', " +
					std::to_string( get_faux_item_id( 40, 921 ) ) +
					");"
					).c_str( )
			);
	
			set_created_model_preview( true );
		}
		
		void set_created_model_preview( bool v ) {
			this->m_created_model_preview = v;
		}

		bool get_created_model_preview( ) const {
			return this->m_created_model_preview;
		}
	private:
		c_texture_dx11* m_current_model_texture;
		bool m_created_model_preview = false;
	}; inline c_model_preview m_model_preview;

	bool esp_preview( ImDrawList* draw_list, c_visuals::bb_t& bbox );
}