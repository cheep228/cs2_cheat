#include "font_manager.h"
#include <imgui_impl_dx11.h>   // PORT: not pulled in by the shim includes.h
#include "compressed_fonts/manrope_regular_compressed.h"
#include "compressed_fonts/smallest_pixel.h"

void c_font_manager::init( ) {
	ImGuiIO& io = ImGui::GetIO( );
	io.Fonts->Clear( );
	{
		static const ImWchar icon_ranges[ ] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

		ImFontConfig fa_font_config;

		constexpr float fa_x = 2.f / 3.f;

		this->m_fa_large = io.Fonts->AddFontFromMemoryCompressedTTF( FA_FONT, FA_compressed_size, 20.f * fa_x, &fa_font_config, icon_ranges );
		this->m_fa_small = io.Fonts->AddFontFromMemoryCompressedTTF( FA_FONT, FA_compressed_size, 15.f * fa_x, &fa_font_config, icon_ranges );

	}

	this->m_calibri_14.load( io, xx( "C:\\Windows\\Fonts\\calibri.ttf" ), 14 );
	this->m_calibri_12.load( io, xx( "C:\\Windows\\Fonts\\calibri.ttf" ), 12 );

	this->m_verdana_12.load( io, xx( "C:\\Windows\\Fonts\\verdana.ttf" ), 12 );
	this->m_verdana_12_bold.load( io, xx( "C:\\Windows\\Fonts\\verdanab.ttf" ), 12 );
	this->m_verdana_14.load( io, xx( "C:\\Windows\\Fonts\\verdana.ttf" ), 14 );
	this->m_verdana_15.load(io, xx("C:\\Windows\\Fonts\\verdana.ttf"), 16);

	this->m_tahoma_bd_12.load( io, xx( "C:\\Windows\\Fonts\\tahomabd.ttf" ), 12 );
	this->m_segoeuib.load(io, xx("C:\\Windows\\Fonts\\segoeuib.ttf"), 20);
	this->m_segoeui_14.load(io, xx("C:\\Windows\\Fonts\\segoeuib.ttf"), 14);

	this->m_inter_bold_14.load(io, (void*)inter_bold_compressed, sizeof(inter_bold_compressed), 14);
	this->m_gheist_medium_14.load(io, (void*)gheist_medium, sizeof(gheist_medium), 14);
	this->m_gheist_thin_14.load(io, (void*)gheist_thin, sizeof(gheist_thin), 14);
	this->m_gheist_medium_12.load(io, (void*)gheist_medium, sizeof(gheist_medium), 12);
	this->m_gheist_thin_12.load(io, (void*)gheist_thin, sizeof(gheist_thin), 12);
	this->m_inter_bold_12.load(io, (void*)inter_bold_compressed, sizeof(inter_bold_compressed), 12);
	this->m_inter_bold_large.load( io, (void*)inter_bold_compressed, sizeof( inter_bold_compressed ), 24 );
	this->m_smallest_pixel.load( io, (void*)smallest_pixel_compressed, sizeof( smallest_pixel_compressed ), 9 );
	ImGuiIO& fooo = ImGui::GetIO();
	this->m_inter_4002.load(fooo, (void*)InterRegular_compressed_data_base85, sizeof(InterRegular_compressed_data_base85), 18);
	this->m_inter_4003.load(fooo, (void*)InterRegular_compressed_data_base85, sizeof(InterRegular_compressed_data_base85), 17);
	this->m_inter_14.load(fooo, (void*)InterRegular_compressed_data_base85, sizeof(InterRegular_compressed_data_base85), 17);
	this->m_inter_400.load(fooo, (void*)InterRegular_compressed_data_base85, sizeof(InterRegular_compressed_data_base85), 16);
	io.Fonts->Build( );
	fooo.Fonts->Build( );

	ImGui_ImplDX11_InvalidateDeviceObjects( );
	ImGui_ImplDX11_CreateDeviceObjects( );
}
