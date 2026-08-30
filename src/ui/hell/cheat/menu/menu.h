#pragma once

#include <includes.h>
#include <cheat/features/visuals/grenade.h>

enum class e_menu_tabs : unsigned int {
	tab_aimbot = 0,
	tab_visuals,
	tab_skins,
	tab_misc,
	tab_config,
	tab_max
};

class c_menu {
public:
	e_menu_tabs m_current_tab;

	bool m_init = false;
	bool m_menu_open = false;
	bool m_input_active = false;
public:
	void init(void);
	void render(void);
	icon_data_t load_avatar_texture();
};
inline auto g_menu = std::make_unique<c_menu>();