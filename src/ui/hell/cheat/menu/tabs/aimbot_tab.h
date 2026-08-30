#pragma once

#include <context.h>

namespace tabs {
	namespace aimbot {
		fnv1a_t get_min_damage_holder_id(int current_weapon);
		fnv1a_t get_hitchance_holder_id(int current_weapon);
		fnv1a_t get_air_hitchance_holder_id(int current_weapon);
		fnv1a_t get_pointscale_holder_id(int current_weapon);
	}
	void render_aimbot();
}