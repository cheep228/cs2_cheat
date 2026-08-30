#include "menu.h"
#include <context.h>
#include <core/interfaces/interfaces.h>

#include "tabs/config_tab.h"
#include "tabs/misc_tab.h"
#include "tabs/visuals_tab.h"
#include "tabs/aimbot_tab.h"

#include "animation/anim.h"

#include <sdk/interfaces/global_variables.h>
#include <utils/fonts/font_manager.h>

#include "hell_gui/hell_gui.h"
#include "hell_gui/colors.h"
#include "tabs/skins_tab.h"
#include <utils/fonts/compressed_fonts/font_awesome.h>
#include <shellapi.h>

#include <utils/stb_image.h>
#include <utils/fonts/compressed_fonts/test_avatar.h>

void c_menu::init(void) {
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(g_ctx->m_window);
	ImGui_ImplDX11_Init(g_interfaces->m_device, g_interfaces->m_device_context);

	g_font_manager->init();

	hell::blur::m_initialized = hell::blur::blur_effect.initialize(g_interfaces->m_device, g_interfaces->m_device_context);

	m_init = true;
}

icon_data_t c_menu::load_avatar_texture() {
	static icon_data_t avatar_texture;

	if (avatar_texture.texture_view)
		return avatar_texture;

	int width, height, channels;
	unsigned char* data = stbi_load_from_memory(test_avatar, sizeof(test_avatar), &width, &height, &channels, 4);
	if (!data)
		return {};

	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* texture_view = nullptr;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sub_resource = {};
	sub_resource.pSysMem = data;
	sub_resource.SysMemPitch = desc.Width * 4;
	sub_resource.SysMemSlicePitch = 0;

	auto device = g_interfaces->m_device;
	device->CreateTexture2D(&desc, &sub_resource, &texture);

	if (texture) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = desc.MipLevels;
		srv_desc.Texture2D.MostDetailedMip = 0;

		device->CreateShaderResourceView(texture, &srv_desc, &texture_view);
		texture->Release();
	}

	stbi_image_free(data);

	if (texture_view) {
		avatar_texture = icon_data_t(texture_view, width, height);
	}

	return avatar_texture;
}

void c_menu::render(void) {
	if (!g_menu->m_menu_open)
		return;

	hell::window::m_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
	ImGui::SetNextWindowContentSize({ 650, 550 });
	hell::begin_window(1, { 650, 550 }, { 25, 25, 25, 220 }, 7.f, true);
	{
		ImGui::PushFont(g_font_manager->m_gheist_medium_14);

		hellvec2 window_pos = ImGui::GetWindowPos();
		hellvec2 window_size = ImGui::GetWindowSize();

		float sidebar_width = 140.f;
		float top_bar_height = 38.f;

		ImRect sidebar_rect = { window_pos, hellvec2(window_pos.x + sidebar_width, window_pos.y + window_size.y) };
		hell::window::m_window_drawlist->AddRectFilled(sidebar_rect.Min, sidebar_rect.Max, hellcolor(25, 25, 25, 150), 7.f, ImDrawFlags_RoundCornersLeft);

		hellvec2 logo_size = hellvec2(sidebar_width, top_bar_height);
		ImRect logo_rect = { sidebar_rect.Min, sidebar_rect.Min + logo_size };
		float tab_size_y_pad = ImGui::GetStyle().WindowPadding.y;

		constexpr const char* logo_text = xx("QUINTCS2");
		constexpr const char* logo_text_first = xx("QUINT");
		constexpr const char* logo_text_second = xx("CS2");

		ImFont* logo_font = g_font_manager->m_inter_bold_large;
		hellvec2 logo_text_size = logo_font->CalcTextSizeA(logo_font->LegacySize, FLT_MAX, 0.f, logo_text);
		hellvec2 logo_pos = { logo_rect.Min.x + (logo_rect.GetWidth() - logo_text_size.x) * 0.5f, logo_rect.Min.y + (logo_rect.GetHeight() - logo_font->GetFontBaked(logo_font->LegacySize)->Ascent) * 0.5f + 3.f };

		hell::label_shadow(logo_pos, logo_font, logo_text, hellcolor(255, 255, 255));

		float first_part_width = logo_font->CalcTextSizeA(logo_font->LegacySize, FLT_MAX, 0.f, logo_text_first).x;
		hellvec2 second_part_pos = { logo_pos.x + first_part_width, logo_pos.y };
		ImGui::GetWindowDrawList()->AddText(logo_font, logo_font->LegacySize, second_part_pos, hellcolor(255, 255, 255), logo_text_second);

		auto avatar_texture = load_avatar_texture();
		{
			constexpr float avatar_size = 26.f;
			constexpr float user_info_padding_x = 10.f;
			constexpr float user_info_padding_y = 12.f;

			const char* username = "Quint cs2!!!";
			const char* until_label = "Version: Beta";


			hellvec2 username_size = g_font_manager->m_gheist_medium_12->CalcTextSizeA(g_font_manager->m_gheist_medium_12->LegacySize, FLT_MAX, 0.f, username);
			hellvec2 until_label_size = g_font_manager->m_gheist_medium_12->CalcTextSizeA(g_font_manager->m_gheist_medium_12->LegacySize, FLT_MAX, 0.f, until_label);

			float block_height = ImMax(avatar_size, username_size.y + until_label_size.y + 1.f);

			hellvec2 avatar_pos = { sidebar_rect.Min.x + user_info_padding_x, sidebar_rect.Max.y - user_info_padding_y - block_height + (block_height - avatar_size) * 0.5f };

			if (avatar_texture.texture_view) {
				hell::window::m_window_drawlist->AddImageRounded((ImTextureID)avatar_texture.texture_view, avatar_pos, { avatar_pos.x + avatar_size, avatar_pos.y + avatar_size }, { 0, 0 }, { 1, 1 }, IM_COL32_WHITE, avatar_size * 0.25f);
			}

			float text_left = avatar_pos.x + avatar_size + 8.f;
			float text_top = sidebar_rect.Max.y - user_info_padding_y - block_height + (block_height - (username_size.y + until_label_size.y + 1.f)) * 0.5f;

			hellvec2 username_pos = { text_left, text_top };
			hellvec2 until_label_pos = { text_left, text_top + username_size.y + 1.f };
			hellvec2 until_date_pos = { until_label_pos.x + until_label_size.x, until_label_pos.y };

			hell::window::m_window_drawlist->AddText(g_font_manager->m_gheist_medium_12, g_font_manager->m_gheist_medium_12->LegacySize, username_pos, hellcolor(255, 255, 255), username);
			hell::window::m_window_drawlist->AddText(g_font_manager->m_gheist_medium_12, g_font_manager->m_gheist_medium_12->LegacySize, until_label_pos, hellcolor(140, 140, 140), until_label);
			
		}

		static std::array<const char*, 5> tab_names = {
			xx("Rage"),
			xx("Render"),
			xx("Skins"),
			xx("Misc"),
			xx("Settings")
		};

		static std::array<const char*, 5> tab_icons = {
			ICON_FA_CROSSHAIRS,
			ICON_FA_EYE,
			ICON_FA_PALETTE,
			ICON_FA_SLIDERS,
			ICON_FA_GEAR
		};

		float tab_start_x = sidebar_rect.Min.x + 10.f;
		float tab_start_y = logo_rect.Max.y + 10.f;
		float tab_width = sidebar_width - 20.f;
		float tab_spacing = 4.f;
		float tab_padding_x = 12.f;
		float tab_padding_y = 8.f;

		hellcolor inactive_color = hellcolor(180, 180, 180, 255);
		hellcolor active_color = hellcolor(255, 255, 255, 255);
		hellcolor bg_color = hellcolor(45, 45, 45, 120);

		float current_y = tab_start_y;

		for (int i = 0; i < 5; i++) {
			bool is_selected = (int)m_current_tab == i;

			ImGui::PushFont(g_font_manager->m_fa_large);
			hellvec2 icon_size = ImGui::CalcTextSize(tab_icons[i]);
			ImGui::PopFont();

			hellvec2 text_size = g_font_manager->m_gheist_medium_14->CalcTextSizeA(g_font_manager->m_gheist_medium_14->LegacySize, FLT_MAX, 0.f, tab_names[i]);

			float total_height = ImMax(icon_size.y, text_size.y) + tab_padding_y * 2.f;

			hellvec2 tab_min = hellvec2(tab_start_x, current_y);
			hellvec2 tab_max = hellvec2(tab_start_x + tab_width, current_y + total_height);

			ImGui::SetCursorScreenPos(tab_min);
			ImGui::InvisibleButton(tab_names[i], hellvec2(tab_width, total_height));

			bool hovered = ImGui::IsItemHovered();
			bool clicked = ImGui::IsItemClicked();

			if (clicked) {
				m_current_tab = (e_menu_tabs)i;
			}

			if (is_selected) {
				hell::window::m_window_drawlist->AddRectFilled(tab_min, tab_max, bg_color, 4.f);
			}

			hellcolor icon_color = is_selected ? hellcolor(255, 255, 255) : (hovered ? hellcolor(200, 200, 200, 255) : inactive_color);
			hellcolor text_color = is_selected ? active_color : (hovered ? hellcolor(220, 220, 220, 255) : inactive_color);

			float row_center_y = current_y + total_height * 0.5f;

			hellvec2 icon_pos = hellvec2(tab_min.x + tab_padding_x, row_center_y - icon_size.y * 0.5f);
			ImGui::PushFont(g_font_manager->m_fa_large);
			hell::window::m_window_drawlist->AddText(icon_pos, icon_color, tab_icons[i]);
			ImGui::PopFont();

			hellvec2 text_pos = hellvec2(icon_pos.x + icon_size.x + 8.f, row_center_y - text_size.y * 0.5f);
			hell::window::m_window_drawlist->AddText(g_font_manager->m_gheist_medium_14, g_font_manager->m_gheist_medium_14->LegacySize, text_pos, text_color, tab_names[i]);

			current_y += total_height + tab_spacing;
		}

		float footer_y_pad = ImGui::GetStyle().WindowPadding.y;
		hell::window::m_footer_height = 0.f;

		ImGui::SetCursorPos({ sidebar_width + 6.f, tab_size_y_pad });

		hellvec2 content_avail = { window_size.x - sidebar_width - 6.f - ImGui::GetStyle().WindowPadding.x, window_size.y - tab_size_y_pad - footer_y_pad };
		ImGui::BeginChild(xx("##quintcs2_content"), content_avail, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
		{
			switch (m_current_tab) {
			case e_menu_tabs::tab_aimbot:
				tabs::render_aimbot();
				break;
			case e_menu_tabs::tab_visuals:
				tabs::render_visuals();
				break;
			case e_menu_tabs::tab_skins:
				tabs::render_skins();
				break;
			case e_menu_tabs::tab_misc:
				tabs::render_misc();
				break;
			case e_menu_tabs::tab_config:
				tabs::render_config();
				break;
			default:
				break;
			}
		}
		ImGui::EndChild();

		ImGui::PopFont();
	}
	hell::end_window();
}