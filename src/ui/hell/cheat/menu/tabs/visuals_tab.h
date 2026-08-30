#pragma once

namespace tabs {
	namespace visuals {
		enum e_subtab {
			subtab_local = 0,
			subtab_enemy,
			subtab_world,
			subtab_other,
			subtab_max
		};
	}

	void render_visuals();
}