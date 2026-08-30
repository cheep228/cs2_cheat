#pragma once

#include <includes.h>

#include "compressed_fonts/inter_bold_compressed.h"
#include "compressed_fonts/font_awesome.h"

class c_font_manager {
private:
	struct font_t {
	private:
		ImFont* m_font;
	public:
		void load( ImGuiIO& io, const char* path, int font_size ) {
			ImFontConfig font_config{};
			// PORT: this project does not build ImGui with freetype, so the
			// ForceAutoHint builder flag has no backend to apply it.

			m_font = io.Fonts->AddFontFromFileTTF(
				path,
				font_size,
				&font_config,
				io.Fonts->GetGlyphRangesCyrillic( )
			);
		}

		void load( ImGuiIO& io, void* data, int data_size, int font_size, bool compressed = false ) {
			ImFontConfig font_config{};
			// PORT: this project does not build ImGui with freetype, so the
			// ForceAutoHint builder flag has no backend to apply it.
			if ( compressed ) {
				m_font = io.Fonts->AddFontFromMemoryCompressedTTF(
					data,
					data_size,
					font_size,
					&font_config,
					io.Fonts->GetGlyphRangesCyrillic( )
				);
			}
			else {
				m_font = io.Fonts->AddFontFromMemoryTTF(
					data,
					data_size,
					font_size,
					&font_config,
					io.Fonts->GetGlyphRangesCyrillic( )
				);
			}
		}

		operator ImFont*( ) const {
			return m_font;
		}

		ImFont* operator->( ) const {
			return m_font;
		}

		font_t& operator=( ImFont* font ) {
			m_font = font;
			return *this;
		}
	};
public:
	font_t m_verdana_12;
	font_t m_verdana_15;
	font_t m_verdana_12_bold;
	font_t m_segoeuib;
	font_t m_gheist_thin_14;
	font_t m_gheist_medium_14;
	font_t m_gheist_medium_12;
	font_t m_gheist_thin_12;
	font_t m_segoeui_14;
	font_t m_inter_400;
	font_t m_inter_4002;
	font_t m_inter_4003;
	font_t m_inter_14;
	font_t m_verdana_14;
	font_t m_calibri_12;
	font_t m_inter_bold_14;
	font_t m_inter_bold_12;
	font_t m_inter_bold_large;
	font_t m_smallest_pixel;
	font_t m_manrope_regular_12;
	font_t m_calibri_14;
	font_t m_fa_large;
	font_t m_fa_small;
	font_t m_tahoma_bd_12;

	void init( );
};

inline auto g_font_manager = std::make_unique<c_font_manager>( );

