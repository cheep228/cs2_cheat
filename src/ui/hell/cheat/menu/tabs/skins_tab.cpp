#include "skins_tab.h"

#include <includes.h>
#include <cheat/config/config_system.h>
#include <cheat/config/vars.h>
#include <cheat/input.h>
#include <unordered_map>
#include "skins_tab.h"

#include <includes.h>
#include <cheat/config/config_system.h>
#include <cheat/config/vars.h>
#include <cheat/input.h>
#include <unordered_map>

#include <cheat/menu/hell_gui/hell_gui.h>
#include <cheat/features/skins/skins.h>
#include <utils/fonts/font_manager.h>
#include <cheat/menu/hell_gui/colors.h>
#include <utils/vtex_parser.h>


// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

int get_skin_index_by_id(const std::vector<c_dumped_skin>& skins, int id) {
	for (int i = 0; i < (int)skins.size(); ++i)
		if (skins[i].id == id)
			return i;
	return 0;
}

hellcolor get_rarity_color(uint32_t rarity) {
	switch (rarity) {
	case 0:  return hellcolor(255, 255, 255, 255); // Consumer
	case 1:  return hellcolor(176, 195, 217, 255); // Industrial
	case 2:  return hellcolor(94, 152, 217, 255); // Mil-Spec
	case 3:  return hellcolor(75, 105, 255, 255); // Restricted
	case 4:  return hellcolor(136, 71, 255, 255); // Classified
	case 5:  return hellcolor(211, 44, 230, 255); // Covert
	case 6:  return hellcolor(235, 75, 75, 255); // Contraband
	case 7:  return hellcolor(228, 174, 57, 255); // Gold
	default: return hellcolor(255, 255, 255, 255);
	}
}

uint32_t get_paint_kit_rarity(int paint_kit_id) {
	auto schema = g_interfaces->m_source2_client->get_econ_item_system()->get_econ_item_schema();
	if (!schema) return 1;
	auto kit = schema->get_paint_kits().find_by_key(paint_kit_id);
	if (kit.has_value()) return kit.value()->paint_kit_rarity();
	return 1;
}

uint32_t get_agent_rarity(int agent_id) {
	auto schema = g_interfaces->m_source2_client->get_econ_item_system()->get_econ_item_schema();
	if (!schema) return 1;
	auto def = schema->get_sorted_item_definition_map().find_by_key(agent_id);
	if (def.has_value()) return def.value()->m_iterarity;
	return 1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Texture cache
// ─────────────────────────────────────────────────────────────────────────────

static std::unordered_map<std::string, ID3D11ShaderResourceView*> g_texture_cache;

ID3D11ShaderResourceView* load_or_get_texture(const std::string& path) {
	if (path.empty()) return nullptr;
	auto it = g_texture_cache.find(path);
	if (it != g_texture_cache.end()) return it->second;

	if (!g_interfaces->m_file_system->exists(path.c_str(), xx("GAME"))) return nullptr;

	auto vtex = vtex_parser::load(path, g_interfaces->m_file_system);
	if (vtex.data.empty()) return nullptr;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = vtex.w;
	desc.Height = vtex.h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sub = {};
	sub.pSysMem = vtex.data.data();
	sub.SysMemPitch = desc.Width * 4;

	ID3D11Texture2D* tex2d = nullptr;
	if (FAILED(g_interfaces->m_device->CreateTexture2D(&desc, &sub, &tex2d))) return nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Texture2D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* srv = nullptr;
	g_interfaces->m_device->CreateShaderResourceView(tex2d, &srv_desc, &srv);
	tex2d->Release();

	if (srv) g_texture_cache[path] = srv;
	return srv;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Inventory slot state
// ─────────────────────────────────────────────────────────────────────────────


enum e_slot_category : int {
	cat_knife = 0,
	cat_glove,
	cat_weapon,
	cat_agent,
	cat_max
};
static const char* s_cat_label[cat_max] = {
	xx("KNIFE"), xx("GLOVE"), xx("WEAPON"), xx("AGENT")
};



// Describes one visible inventory slot
struct inv_slot_t {
	e_slot_category category = cat_knife;
	int             sub_idx = 0; // knife/glove/weapon index, or agent list index
	int             side = 0; // cat_agent only: 0=CT, 1=T
};

static std::vector<inv_slot_t> g_inv_slots;
static int  g_selected_slot = -1;
static bool g_picker_open = false;
static int  g_picker_category = cat_knife;
static int  g_picker_model = 0;
static int  g_picker_skin = -1;
static bool g_initialized = false;

// ─────────────────────────────────────────────────────────────────────────────
//  Text truncation helper
// ─────────────────────────────────────────────────────────────────────────────

// Returns text clipped to fit within max_width pixels using the current font.
// Appends "…" (U+2026) if truncated.
static std::string truncate_text(const std::string& text, float max_width) {
	ImFont* font = ImGui::GetFont();
	float   ellipsis = font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, xx("...")).x;

	if (font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.f, text.c_str()).x <= max_width)
		return text;

	// Binary-search for the longest prefix that fits
	int lo = 0, hi = (int)text.size();
	while (lo < hi) {
		int   mid = (lo + hi + 1) / 2;
		float test_w = font->CalcTextSizeA(
			font->LegacySize, FLT_MAX, 0.f,
			text.c_str(), text.c_str() + mid
		).x;
		if (test_w + ellipsis <= max_width) lo = mid; else hi = mid - 1;
	}
	return text.substr(0, lo) + xx("...");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Initialize inventory slots from saved config
// ─────────────────────────────────────────────────────────────────────────────

static void initialize_inventory_slots() {
	g_inv_slots.clear();

	int knife_idx = GET_VAR(int, SKINS_PATH(m_knife_selected));
	auto& knife_settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_knives_settings));
	if (knife_idx >= 0 && knife_idx < (int)g_skins->knives_items.size() &&
		knife_idx < (int)knife_settings.size() && knife_settings[knife_idx].m_paint_kit > 0) {
		g_inv_slots.push_back({ cat_knife, knife_idx });
	}

	int glove_idx = GET_VAR(int, SKINS_PATH(m_glove_selected));
	auto& glove_settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_glove_settings));
	if (glove_idx >= 0 && glove_idx < (int)g_skins->glove_items.size() &&
		glove_idx < (int)glove_settings.size() && glove_settings[glove_idx].m_paint_kit > 0) {
		g_inv_slots.push_back({ cat_glove, glove_idx });
	}

	auto& weapon_settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_skin_settings));
	for (int i = 0; i < (int)weapon_settings.size() && i < (int)g_skins->items.size(); ++i) {
		if (weapon_settings[i].m_paint_kit > 0)
			g_inv_slots.push_back({ cat_weapon, i });
	}

	int ct_agent = GET_VAR(int, SKINS_PATH(m_agent_selected_ct));
	if (ct_agent > 0 && ct_agent < (int)g_skins->agents.agent_names.size()) {
		inv_slot_t s; s.category = cat_agent; s.sub_idx = ct_agent; s.side = 0;
		g_inv_slots.push_back(s);
	}

	int t_agent = GET_VAR(int, SKINS_PATH(m_agent_selected_t));
	if (t_agent > 0 && t_agent < (int)g_skins->agents.agent_names.size()) {
		inv_slot_t s; s.category = cat_agent; s.sub_idx = t_agent; s.side = 1;
		g_inv_slots.push_back(s);
	}

	g_selected_slot = g_inv_slots.empty() ? -1 : 0;
	g_initialized = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Texture / label resolvers  (unchanged logic)
// ─────────────────────────────────────────────────────────────────────────────

static std::string resolve_slot_texture(const inv_slot_t& slot) {
	auto schema = g_interfaces->m_source2_client->get_econ_item_system()->get_econ_item_schema();
	if (!schema) return {};

	auto make_path = [&](int def_index, int paint_kit_id) -> std::string {
		auto def_opt = schema->get_sorted_item_definition_map().find_by_key(def_index);
		auto kit_opt = schema->get_paint_kits().find_by_key(paint_kit_id);
		if (!def_opt.has_value() || !kit_opt.has_value()) return {};
		return "panorama/images/econ/default_generated/"
			+ std::string(def_opt.value()->get_simple_weapon_name()) + "_"
			+ kit_opt.value()->paint_kit_name()
			+ "_light_png.vtex_c";
		};

	switch (slot.category) {
	case cat_knife: {
		if (slot.sub_idx >= (int)g_skins->knives_items.size()) return {};
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_knives_settings));
		if (slot.sub_idx >= (int)settings.size()) return {};
		auto& skins = g_skins->knives_items[slot.sub_idx].skins;
		int   idx = get_skin_index_by_id(skins, settings[slot.sub_idx].m_paint_kit);
		if (skins.empty() || idx < 0 || idx >= (int)skins.size()) return {};
		return make_path(g_skins->knives_items[slot.sub_idx].def_index, skins[idx].id);
	}
	case cat_glove: {
		if (slot.sub_idx >= (int)g_skins->glove_items.size()) return {};
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_glove_settings));
		if (slot.sub_idx >= (int)settings.size()) return {};
		auto& skins = g_skins->glove_items[slot.sub_idx].skins;
		int   idx = get_skin_index_by_id(skins, settings[slot.sub_idx].m_paint_kit);
		if (skins.empty() || idx < 0 || idx >= (int)skins.size()) return {};
		return make_path(g_skins->glove_items[slot.sub_idx].def_index, skins[idx].id);
	}
	case cat_weapon: {
		auto& items = g_skins->items;
		if (slot.sub_idx >= (int)items.size()) return {};
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_skin_settings));
		if (slot.sub_idx >= (int)settings.size()) return {};
		auto& skins = items[slot.sub_idx].skins;
		int   idx = get_skin_index_by_id(skins, settings[slot.sub_idx].m_paint_kit);
		if (skins.empty() || idx < 0 || idx >= (int)skins.size()) return {};
		return make_path(items[slot.sub_idx].def_index, skins[idx].id);
	}
	case cat_agent: {
		auto& agents = g_skins->agents;
		if (slot.sub_idx >= (int)agents.agent_ids.size()) return {};
		int agent_id = agents.agent_ids[slot.sub_idx];
		auto schema2 = g_interfaces->m_source2_client->get_econ_item_system()->get_econ_item_schema();
		if (!schema2) return {};
		auto def_opt = schema2->get_sorted_item_definition_map().find_by_key(agent_id);
		if (!def_opt.has_value()) return {};
		std::string name = def_opt.value()->get_simple_weapon_name();
		for (auto& p : {
				"panorama/images/econ/default_generated/" + name + "_png.vtex_c",
				"panorama/images/econ/characters/" + name + ".vtex_c",
				"panorama/images/econ/characters/" + name + "_png.vtex_c",
				"panorama/images/econ/default_generated/" + name + ".vtex_c" }) {
			if (g_interfaces->m_file_system->exists(p.c_str(), xx("GAME"))) return p;
		}
		return {};
	}
	}
	return {};
}

// Returns "Model | Skin" or just "Model"
static std::string resolve_slot_label(const inv_slot_t& slot) {
	switch (slot.category) {
	case cat_knife: {
		if (slot.sub_idx >= (int)g_skins->knives_items.size()) return xx("Knife");
		std::string model_name = g_skins->knives.names[slot.sub_idx];
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_knives_settings));
		if (slot.sub_idx >= (int)settings.size()) return model_name;
		auto& skins = g_skins->knives_items[slot.sub_idx].skins;
		int   idx = get_skin_index_by_id(skins, settings[slot.sub_idx].m_paint_kit);
		if (skins.empty() || idx < 0 || idx >= (int)skins.size()) return model_name;
		return model_name + xx(" | ") + skins[idx].name;
	}
	case cat_glove: {
		if (slot.sub_idx >= (int)g_skins->glove_items.size()) return xx("Glove");
		std::string model_name = g_skins->gloves.names[slot.sub_idx];
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_glove_settings));
		if (slot.sub_idx >= (int)settings.size()) return model_name;
		auto& skins = g_skins->glove_items[slot.sub_idx].skins;
		int   idx = get_skin_index_by_id(skins, settings[slot.sub_idx].m_paint_kit);
		if (skins.empty() || idx < 0 || idx >= (int)skins.size()) return model_name;
		return model_name + xx(" | ") + skins[idx].name;
	}
	case cat_weapon: {
		auto& items = g_skins->items;
		if (slot.sub_idx >= (int)items.size()) return xx("Weapon");
		std::string model_name = items[slot.sub_idx].name;
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_skin_settings));
		if (slot.sub_idx >= (int)settings.size()) return model_name;
		auto& skins = items[slot.sub_idx].skins;
		int   idx = get_skin_index_by_id(skins, settings[slot.sub_idx].m_paint_kit);
		if (skins.empty() || idx < 0 || idx >= (int)skins.size()) return model_name;
		return model_name + xx(" | ") + skins[idx].name;
	}
	case cat_agent: {
		auto& agents = g_skins->agents;
		if (slot.sub_idx >= (int)agents.agent_names.size()) return xx("Agent");
		const char* side_prefix = slot.side == 0 ? xx("[CT] ") : xx("[T]  ");
		return std::string(side_prefix) + agents.agent_names[slot.sub_idx];
	}
	}
	return {};
}

// Returns just the weapon/model part before " | "
static std::string slot_model_name(const std::string& full_label) {
	auto pos = full_label.find(xx(" | "));
	return (pos != std::string::npos) ? full_label.substr(0, pos) : full_label;
}

// Returns just the skin part after " | ", empty if none
static std::string slot_skin_name(const std::string& full_label) {
	auto pos = full_label.find(xx(" | "));
	return (pos != std::string::npos) ? full_label.substr(pos + 3) : std::string{};
}

// ─────────────────────────────────────────────────────────────────────────────
//  Rarity resolver for a slot (used for the accent strip)
// ─────────────────────────────────────────────────────────────────────────────

static uint32_t resolve_slot_rarity(const inv_slot_t& slot) {
	switch (slot.category) {
	case cat_knife: {
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_knives_settings));
		if (slot.sub_idx >= (int)settings.size()) return 1;
		return get_paint_kit_rarity(settings[slot.sub_idx].m_paint_kit);
	}
	case cat_glove: {
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_glove_settings));
		if (slot.sub_idx >= (int)settings.size()) return 1;
		return get_paint_kit_rarity(settings[slot.sub_idx].m_paint_kit);
	}
	case cat_weapon: {
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_skin_settings));
		if (slot.sub_idx >= (int)settings.size()) return 1;
		return get_paint_kit_rarity(settings[slot.sub_idx].m_paint_kit);
	}
	case cat_agent: {
		auto& agents = g_skins->agents;
		if (slot.sub_idx >= (int)agents.agent_ids.size()) return 1;
		return get_agent_rarity(agents.agent_ids[slot.sub_idx]);
	}
	}
	return 1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  skin_selectable  (unchanged)
// ─────────────────────────────────────────────────────────────────────────────

bool skin_selectable(const char* label, bool selected, hellcolor text_color) {
	ImGuiWindow* im_window = ImGui::GetCurrentWindow();
	if (im_window->SkipItems) return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& im_style = g.Style;

	hellvec2 im_label_size = ImGui::CalcTextSize(label, NULL, true);
	hellvec2 im_avail_region = ImGui::GetContentRegionAvail();
	hellvec2 im_item_size(im_avail_region.x, im_label_size.y);

	hellvec2 im_pos = im_window->DC.CursorPos;
	ImGui::ItemSize(im_item_size, 0.0f);

	ImRect im_bb(im_pos, im_pos + im_item_size);
	if (!ImGui::ItemAdd(im_bb, im_window->GetID(label))) return false;

	bool b_hovered, b_held;
	bool b_pressed = ImGui::ButtonBehavior(im_bb, im_window->GetID(label), &b_hovered, &b_held);

	ImDrawList* im_draw_list = im_window->DrawList;
	hellvec2 im_text_pos = { im_bb.Min.x, im_bb.GetCenter().y - (im_label_size.y * 0.5f) };

	std::string str_new_label(label);
	size_t pos = str_new_label.find(xx("##"));
	if (pos != std::string::npos) str_new_label = str_new_label.substr(0, pos);

	ImGuiID id = im_window->GetID(label);

	float* select_progress = ImGui::GetStateStorage()->GetFloatRef(id, 0.0f);
	float* hover_progress = ImGui::GetStateStorage()->GetFloatRef(id + 1, 0.0f);
	float* color_transition = ImGui::GetStateStorage()->GetFloatRef(id + 2, 0.0f);

	*select_progress = ImSaturate(*select_progress + g.IO.DeltaTime * (selected ? 15.f : -15.f));
	*hover_progress = ImSaturate(*hover_progress + g.IO.DeltaTime * (b_hovered && !selected ? 15.f : -15.f));

	hellcolor target_color = text_color;
	if (selected)      target_color = hellcolor(140, 90, 190, 255);
	else if (b_hovered) target_color = text_color;

	*color_transition = ImLerp(*color_transition, target_color.Value.w, g.IO.DeltaTime * 10.f);
	target_color.Value.w = *color_transition;

	float x_offset = ImLerp(0.f, 3.3f, *select_progress) + ImLerp(0.f, 1.5f, *hover_progress);
	im_text_pos.x += x_offset;

	im_draw_list->AddText(im_text_pos, target_color, str_new_label.c_str());
	return b_pressed;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Picker modal
// ─────────────────────────────────────────────────────────────────────────────
static void render_picker(int target_slot_idx) {

	if (!g_skins) {
		ImGui::TextUnformatted(xx("Loading skins data..."));
		return;
	}

	// ── Category buttons ──────────────────────────────────────────────
	hell::label(xx("Type"));
	ImGui::Spacing();

	for (int i = 0; i < cat_max; ++i) {
		ImGui::PushID(i);
		if (hell::button(s_cat_label[i])) {
			g_picker_category = i;
			g_picker_model = 0;
			g_picker_skin = -1;
		}
		if ((i + 1) % 2 != 0)
			ImGui::SameLine();
		ImGui::PopID();
	}

	ImGui::Separator();

	// ── Selected skin hint (header-row) ───────────────────────────────
	{
		const char* hint = xx("Select a skin...");
		std::string hint_buf;

		if (g_picker_skin >= 0 && g_picker_model >= 0) {
			switch (g_picker_category) {
			case cat_knife:
				if (g_picker_model < (int)g_skins->knives_items.size() &&
					g_picker_model < (int)g_skins->knives.names.size() &&
					g_picker_skin < (int)g_skins->knives_items[g_picker_model].skins.size())
				{
					hint_buf = g_skins->knives.names[g_picker_model];
					hint_buf += "  |  ";
					hint_buf += g_skins->knives_items[g_picker_model].skins[g_picker_skin].name;
					hint = hint_buf.c_str();
				}
				break;
			case cat_glove:
				if (g_picker_model < (int)g_skins->glove_items.size() &&
					g_picker_model < (int)g_skins->gloves.names.size() &&
					g_picker_skin < (int)g_skins->glove_items[g_picker_model].skins.size())
				{
					hint_buf = g_skins->gloves.names[g_picker_model];
					hint_buf += "  |  ";
					hint_buf += g_skins->glove_items[g_picker_model].skins[g_picker_skin].name;
					hint = hint_buf.c_str();
				}
				break;
			case cat_weapon:
				if (g_picker_model < (int)g_skins->items.size() &&
					g_picker_skin < (int)g_skins->items[g_picker_model].skins.size())
				{
					hint_buf = g_skins->items[g_picker_model].name;
					hint_buf += "  |  ";
					hint_buf += g_skins->items[g_picker_model].skins[g_picker_skin].name;
					hint = hint_buf.c_str();
				}
				break;
			case cat_agent:
				if (g_picker_skin < (int)g_skins->agents.agent_names.size()) {
					hint_buf = (g_picker_model == 0) ? "CT" : "T";
					hint_buf += "  |  ";
					hint_buf += g_skins->agents.agent_names[g_picker_skin];
					hint = hint_buf.c_str();
				}
				break;
			}
		}

		ImGui::TextDisabled("%s", hint);
	}

	ImGui::Separator();

	// ── Lists (models + skins) ────────────────────────────────────────
	float avail_w = ImGui::GetContentRegionAvail().x;

	float wear_reserve = (g_picker_category == cat_knife || g_picker_category == cat_weapon)
		? ImGui::GetFrameHeightWithSpacing() * 2.f + ImGui::GetStyle().ItemSpacing.y
		: 0.f;
	float footer_h = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y * 2.f + 1.f;
	float list_h = ImGui::GetContentRegionAvail().y - wear_reserve - footer_h - ImGui::GetStyle().ItemSpacing.y;
	list_h = ImMax(list_h, 80.f);

	float col_w = avail_w * 0.38f;
	float skin_w = avail_w - col_w - ImGui::GetStyle().ItemSpacing.x;

	// Model list
	ImGui::BeginGroup();
	{
		hell::label(xx("Model"));
		hell::child(xx("##picker_models"), { col_w, list_h }, [&] {
			switch (g_picker_category) {
			case cat_knife:
				for (int i = 0; i < (int)g_skins->knives.names.size(); ++i)
					if (hell::selectable(g_skins->knives.names[i], g_picker_model == i))
						g_picker_model = i, g_picker_skin = -1;
				break;
			case cat_glove:
				for (int i = 0; i < (int)g_skins->gloves.names.size(); ++i)
					if (hell::selectable(g_skins->gloves.names[i], g_picker_model == i))
						g_picker_model = i, g_picker_skin = -1;
				break;
			case cat_weapon:
				for (int i = 0; i < (int)g_skins->items.size(); ++i)
					if (hell::selectable(g_skins->items[i].name.c_str(), g_picker_model == i))
						g_picker_model = i, g_picker_skin = -1;
				break;
			case cat_agent:
				if (hell::selectable(xx("CT Agents"), g_picker_model == 0)) { g_picker_model = 0; g_picker_skin = -1; }
				if (hell::selectable(xx("T Agents"), g_picker_model == 1)) { g_picker_model = 1; g_picker_skin = -1; }
				break;
			}
			});
	}
	ImGui::EndGroup();
	ImGui::SameLine();

	// Skin list
	ImGui::BeginGroup();
	{
		hell::label(xx("Skin"));
		hell::child(xx("##picker_skins"), { skin_w, list_h }, [&] {
			std::vector<c_dumped_skin>* skin_list = nullptr;
			switch (g_picker_category) {
			case cat_knife:
				if (g_picker_model >= 0 && g_picker_model < (int)g_skins->knives_items.size())
					skin_list = &g_skins->knives_items[g_picker_model].skins;
				break;
			case cat_glove:
				if (g_picker_model >= 0 && g_picker_model < (int)g_skins->glove_items.size())
					skin_list = &g_skins->glove_items[g_picker_model].skins;
				break;
			case cat_weapon:
				if (g_picker_model >= 0 && g_picker_model < (int)g_skins->items.size())
					skin_list = &g_skins->items[g_picker_model].skins;
				break;
			case cat_agent:
		
				if (g_skins->agents.agent_names.size() == g_skins->agents.agent_ids.size()) {
					for (int i = 0; i < (int)g_skins->agents.agent_names.size(); ++i) {
						hellcolor col = get_rarity_color(get_agent_rarity(g_skins->agents.agent_ids[i]));
						ImGui::PushID(i);
						if (skin_selectable(g_skins->agents.agent_names[i], g_picker_skin == i, col))
							g_picker_skin = i;
						ImGui::PopID();
					}
				}
				break;
			}

			if (skin_list) {
				std::unordered_map<std::string, int> name_cnt;
				for (int i = 0; i < (int)skin_list->size(); ++i) {
					hellcolor col = get_rarity_color((*skin_list)[i].rarity);
					std::string dname = (*skin_list)[i].name;
					name_cnt[dname]++;
					if (name_cnt[dname] > 1) dname += " " + std::to_string(name_cnt[dname]);
					ImGui::PushID(i);
					if (skin_selectable(dname.c_str(), g_picker_skin == i, col))
						g_picker_skin = i;
					ImGui::PopID();
				}
			}
			});
	}
	ImGui::EndGroup();

	// ── Wear / Pattern sliders ────────────────────────────────────────
	if (g_picker_category == cat_knife || g_picker_category == cat_weapon) {
		ImGui::Separator();
		const float saved_w = hell::config::m_widget_width;
		hell::config::m_widget_width = 80.f;

		if (g_picker_category == cat_knife) {
			auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_knives_settings));
			if (g_picker_model >= 0) {
				while ((int)settings.size() <= g_picker_model) settings.emplace_back();
				hell::slider_float_passthrough(xx("Wear"), &settings[g_picker_model].m_wear, 0.01f, 1.f, 0, false, "%.2f");
				hell::slider_int_passthrough(xx("Pattern"), &settings[g_picker_model].m_seed, 0, 1000);
			}
		}
		else {
			auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_skin_settings));
			if (g_picker_model >= 0) {
				while ((int)settings.size() <= g_picker_model) settings.emplace_back();
				hell::slider_float_passthrough(xx("Wear"), &settings[g_picker_model].m_wear, 0.01f, 1.f, 0, false, "%.2f");
				hell::slider_int_passthrough(xx("Pattern"), &settings[g_picker_model].m_seed, 0, 1000);
			}
		}

		hell::config::m_widget_width = saved_w;
	}

	// ── Footer: buttons ───────────────────────────────────────────────
	ImGui::Separator();

	bool can_confirm = (g_picker_skin >= 0 && g_picker_model >= 0);

	if (!can_confirm) ImGui::BeginDisabled();
	if (hell::button(xx("Add to Inventory"))) {
		inv_slot_t ns;
		ns.category = (e_slot_category)g_picker_category;
		ns.sub_idx = g_picker_model;

		switch (ns.category) {
		case cat_knife: {
			if (g_picker_model < (int)g_skins->knives_items.size()) {
				auto& skins = g_skins->knives_items[g_picker_model].skins;
				if (g_picker_skin >= 0 && g_picker_skin < (int)skins.size()) {
					auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_knives_settings));
					while ((int)settings.size() <= g_picker_model) settings.emplace_back();
					settings[g_picker_model].m_paint_kit = skins[g_picker_skin].id;
					GET_VAR(int, SKINS_PATH(m_knife_selected)) = g_picker_model;
				}
			}
			break;
		}
		case cat_glove: {
			if (g_picker_model < (int)g_skins->glove_items.size()) {
				auto& skins = g_skins->glove_items[g_picker_model].skins;
				if (g_picker_skin >= 0 && g_picker_skin < (int)skins.size()) {
					auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_glove_settings));
					while ((int)settings.size() <= g_picker_model) settings.emplace_back();
					settings[g_picker_model].m_paint_kit = skins[g_picker_skin].id;
					GET_VAR(int, SKINS_PATH(m_glove_selected)) = g_picker_model;
				}
			}
			break;
		}
		case cat_weapon: {
			if (g_picker_model < (int)g_skins->items.size()) {
				auto& skins = g_skins->items[g_picker_model].skins;
				if (g_picker_skin >= 0 && g_picker_skin < (int)skins.size()) {
					auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_skin_settings));
					while ((int)settings.size() <= g_picker_model) settings.emplace_back();
					settings[g_picker_model].m_paint_kit = skins[g_picker_skin].id;
					GET_VAR(int, SKINS_PATH(m_selected_weapon)) = g_picker_model;
				}
			}
			break;
		}
		case cat_agent: {
			if (g_picker_skin < (int)g_skins->agents.agent_names.size()) {
				ns.sub_idx = g_picker_skin;
				ns.side = g_picker_model;
				if (g_picker_model == 0)
					GET_VAR(int, SKINS_PATH(m_agent_selected_ct)) = g_picker_skin;
				else
					GET_VAR(int, SKINS_PATH(m_agent_selected_t)) = g_picker_skin;
			}
			break;
		}
		}

		bool slot_exists = false;
		for (auto& existing : g_inv_slots) {
			if (existing.category != ns.category) continue;
			if (ns.category == cat_agent) {
				if (existing.side == ns.side) { slot_exists = true; break; }
			}
			else {
				if (existing.sub_idx == ns.sub_idx) { slot_exists = true; break; }
			}
		}
		if (!slot_exists) {
			g_inv_slots.push_back(ns);
			g_selected_slot = (int)g_inv_slots.size() - 1;
		}

		g_picker_open = false;
	}
	if (!can_confirm) ImGui::EndDisabled();

	ImGui::SameLine();
	if (hell::button(xx("Cancel")))
		g_picker_open = false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slot detail panel  (right side)
// ─────────────────────────────────────────────────────────────────────────────

static void render_slot_detail(int slot_idx) {
	if (slot_idx < 0 || slot_idx >= (int)g_inv_slots.size()) {
		// Empty-state hint
		hellvec2 avail = ImGui::GetContentRegionAvail();
		const char* hint = xx("Select a slot\nor add one with +");
		hellvec2 ts = ImGui::CalcTextSize(hint);
		ImGui::SetCursorPos({
			(avail.x - ts.x) * 0.5f,
			avail.y * 0.4f
			});
		ImGui::TextDisabled("%s", hint);
		return;
	}

	auto& slot = g_inv_slots[slot_idx];

	// ── Skin preview ──────────────────────────────────────────────────────
	std::string tex_path = resolve_slot_texture(slot);
	auto        srv = load_or_get_texture(tex_path);

	{
		float    preview_h = 130.f;
		hellvec2 avail = ImGui::GetContentRegionAvail();
		hellvec2 prev_pos = { ImGui::GetCursorPosX(), ImGui::GetCursorPosY() };
		ImGui::Dummy({ avail.x, preview_h });

		if (srv) {
			ID3D11Texture2D* tex2d = nullptr;
			((ID3D11ShaderResourceView*)srv)->GetResource((ID3D11Resource**)&tex2d);
			D3D11_TEXTURE2D_DESC d; tex2d->GetDesc(&d); tex2d->Release();

			float    scale = std::min(avail.x / (float)d.Width, preview_h / (float)d.Height);
			hellvec2 isz = { d.Width * scale, d.Height * scale };
			hellvec2 ipos = { prev_pos.x + (avail.x - isz.x) * 0.5f,
							   prev_pos.y + (preview_h - isz.y) * 0.5f };
			ImGui::SetCursorPos(ipos);
			ImGui::Image((void*)srv, isz);
		}
		ImGui::SetCursorPosY(prev_pos.y + preview_h + ImGui::GetStyle().ItemSpacing.y);
	}

	// ── Rarity accent line ────────────────────────────────────────────────
	{
		uint32_t rarity = resolve_slot_rarity(slot);
		hellcolor rar_col = get_rarity_color(rarity);
		hellvec2 sep_min = ImGui::GetCursorScreenPos();
		float    sep_width = ImGui::GetContentRegionAvail().x;

		// Thin coloured rule (2 px) instead of a boring grey separator
		ImGui::GetWindowDrawList()->AddRectFilled(
			sep_min,
			{ sep_min.x + sep_width, sep_min.y + 2.f },
			rar_col
		);
		ImGui::Dummy({ sep_width, 2.f });
		ImGui::Spacing();
	}

	// ── Name block: model on top, skin name in accent below ──────────────
	{
		std::string full_label = resolve_slot_label(slot);
		std::string model_part = slot_model_name(full_label);
		std::string skin_part = slot_skin_name(full_label);
		float       avail_w = ImGui::GetContentRegionAvail().x;

		// Model name — white, truncated
		hell::label(truncate_text(model_part, avail_w).c_str());

		// Skin name — purple accent, smaller
		if (!skin_part.empty()) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.45f, 0.90f, 1.f));
			ImGui::TextUnformatted(truncate_text(skin_part, avail_w).c_str());
			ImGui::PopStyleColor();
		}


	}

	ImGui::Separator();

	// ── Per-slot wear / pattern ───────────────────────────────────────────
	const float saved_w = hell::config::m_widget_width;
	hell::config::m_widget_width = 80.f;

	switch (slot.category) {
	case cat_knife: {
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_knives_settings));
		while ((int)settings.size() <= slot.sub_idx) settings.emplace_back();
		auto& s = settings[slot.sub_idx];
		hell::slider_float_passthrough(xx("Wear"), &s.m_wear, 0.01f, 1.f, 0, false, "%.2f");
		hell::slider_int_passthrough(xx("Pattern"), &s.m_seed, 0, 1000);
		break;
	}
	case cat_weapon: {
		auto& settings = GET_VAR_VEC(c_vars::skins_t::skin_settings_t, SKINS_PATH(m_skin_settings));
		while ((int)settings.size() <= slot.sub_idx) settings.emplace_back();
		auto& s = settings[slot.sub_idx];
		hell::slider_float_passthrough(xx("Wear"), &s.m_wear, 0.01f, 1.f, 0, false, "%.2f");
		hell::slider_int_passthrough(xx("Pattern"), &s.m_seed, 0, 1000);
		break;
	}
	case cat_glove:
		// No wear/pattern
		break;
	case cat_agent:
		// Show which side this agent is for
		hell::label(slot.side == 0 ? xx("Counter-Terrorist") : xx("Terrorist"));
		break;
	}

	hell::config::m_widget_width = saved_w;

	ImGui::Separator();

	// ── Actions ───────────────────────────────────────────────────────────
	if (hell::button(xx("Remove"))) {
		g_inv_slots.erase(g_inv_slots.begin() + slot_idx);
		g_selected_slot = g_inv_slots.empty()
			? -1
			: std::min(g_selected_slot, (int)g_inv_slots.size() - 1);
	}
	ImGui::SameLine();

}

// ─────────────────────────────────────────────────────────────────────────────
//  Main entry point
// ─────────────────────────────────────────────────────────────────────────────

void tabs::render_skins() {
	if (!g_ctx->m_dumped_skins) {
		hellvec2 avail = ImGui::GetContentRegionAvail();
		const char* text = xx("Dumping skins..");
		hellvec2 ts = ImGui::CalcTextSize(text);
		ImGui::SetCursorPos(avail * 0.5f - ts * 0.5f);
		hell::label(text);
		return;
	}

	hell::set_cursor_pos_relative_y();



	ImGui::Spacing();

	if (!g_initialized) initialize_inventory_slots();
	if (tabs::m_force_gui_refresh)
	{
		g_initialized = false;
		g_inv_slots.clear();
		g_selected_slot = -1;

		initialize_inventory_slots();

		tabs::m_force_gui_refresh = false;
	}
	// ── Layout ────────────────────────────────────────────────────────────
	hellvec2 total_avail = ImGui::GetContentRegionAvail();
	float    detail_w = 220.f;
	float    grid_w = total_avail.x - detail_w - ImGui::GetStyle().WindowPadding.x;
	float    panel_h = hell::get_available_height() - 10.f;

	float start_y = ImGui::GetCursorPosY();
	float start_x = ImGui::GetCursorPosX();

	// ── Inventory grid ────────────────────────────────────────────────────
	hell::child(xx("##Inventory"), { grid_w, panel_h }, [&] {
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.f);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.55f, 0.35f, 0.75f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.55f, 0.35f, 0.75f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.55f, 0.35f, 0.75f, 0.9f));

		const float slot_size = 86.f;
		const float slot_gap = 8.f;
		const float content_w = ImGui::GetContentRegionAvail().x;
		int         cols = std::max(1, (int)((content_w + slot_gap) / (slot_size + slot_gap)));

		// ── Constants for footer label area ──
		// We reserve the bottom ~18 px of each card for two text lines.
		const float footer_h = 18.f; // total footer height
		const float footer_font = ImGui::GetFont()->LegacySize * 0.72f; // small
		const float inner_pad = 4.f;  // horizontal padding inside card
		const float img_area = slot_size - footer_h;  // pixels available for image

		int col = 0;
		for (int i = 0; i < (int)g_inv_slots.size(); ++i) {
			auto& slot = g_inv_slots[i];
			bool  is_sel = (g_selected_slot == i);

			std::string tex_path = resolve_slot_texture(slot);
			auto        srv = load_or_get_texture(tex_path);

			hellvec2 card_pos = ImGui::GetCursorPos();
			ImGui::PushID(i);

			bool clicked = ImGui::InvisibleButton(xx("##slot"), { slot_size, slot_size });
			bool hovered = ImGui::IsItemHovered();

			ImDrawList* dl = ImGui::GetWindowDrawList();
			hellvec2    wpos = ImGui::GetWindowPos();
			hellvec2    spos = card_pos + wpos - hellvec2(0.f, ImGui::GetScrollY());
			hellvec2    epos = spos + hellvec2(slot_size, slot_size);

			// ── Card background & border ──────────────────────────────────
			hellcolor bg_col = is_sel ? hellcolor(60, 30, 90, 200)
				: hovered ? hellcolor(45, 20, 75, 180)
				: hellcolor(30, 15, 55, 160);
			hellcolor brd_col = is_sel ? hellcolor(140, 90, 200, 255)
				: hovered ? hellcolor(90, 60, 140, 200)
				: hellcolor(60, 40, 90, 120);

			dl->AddRect(spos, epos, brd_col, 6.f, 0, is_sel ? 1.5f : 1.f);



			// ── Skin texture ──────────────────────────────────────────────
			if (srv) {
				ID3D11Texture2D* tex2d = nullptr;
				((ID3D11ShaderResourceView*)srv)->GetResource((ID3D11Resource**)&tex2d);
				D3D11_TEXTURE2D_DESC d; tex2d->GetDesc(&d); tex2d->Release();

				float    margin = 10.f;
				float    max_img_w = slot_size - margin * 2.f;
				float    max_img_h = img_area - margin;
				float    img_scale = std::min(max_img_w / (float)d.Width,
					max_img_h / (float)d.Height);
				hellvec2 isz = { d.Width * img_scale, d.Height * img_scale };
				hellvec2 ipos = {
					spos.x + (slot_size - isz.x) * 0.5f,
					spos.y + 3.f + (img_area - 3.f - isz.y) * 0.5f // below rarity strip
				};
				dl->AddImage((void*)srv, ipos, ipos + isz);
			}
			else {
				// Placeholder "?"
				hellvec2 ctr = spos + hellvec2(slot_size * 0.5f, img_area * 0.5f);
				dl->AddText(ctr - hellvec2(5.f, 8.f),
					hellcolor(180, 180, 180, 100),
					xx("?"));
			}


			if (clicked) g_selected_slot = i;
			ImGui::PopID();

			++col;
			if (col < cols) ImGui::SameLine(0.f, slot_gap);
			else { col = 0; ImGui::Spacing(); }
		}

		// ── "+" add button ─────────────────────────────────────────────────
		if (col > 0) ImGui::SameLine(0.f, slot_gap);

		hellvec2 plus_pos = ImGui::GetCursorPos();
		bool     plus_clicked = ImGui::InvisibleButton(xx("##add_slot"), { slot_size, slot_size });
		bool     plus_hovered = ImGui::IsItemHovered();

		ImDrawList* dl = ImGui::GetWindowDrawList();
		hellvec2    wpos = ImGui::GetWindowPos();
		hellvec2    sp = plus_pos + wpos - hellvec2(0.f, ImGui::GetScrollY());
		hellvec2    ep = sp + hellvec2(slot_size, slot_size);

		hellcolor plus_bg = plus_hovered ? hellcolor(50, 25, 80, 180) : hellcolor(30, 15, 55, 100);
		hellcolor plus_brd = plus_hovered ? hellcolor(140, 90, 200, 200) : hellcolor(80, 50, 120, 100);

		// Dashed-style border via 4 short rects on each side
		dl->AddRectFilled(sp, ep, plus_bg, 6.f);
		dl->AddRect(sp, ep, plus_brd, 6.f, 0, 1.f);

		// "+" symbol
		hellvec2  ctr = sp + hellvec2(slot_size * 0.5f, slot_size * 0.5f);
		float     arm = 10.f;
		float     thick = 2.f;
		hellcolor plus_col = plus_hovered ? hellcolor(180, 130, 240, 255) : hellcolor(130, 90, 190, 180);
		dl->AddLine({ ctr.x - arm, ctr.y }, { ctr.x + arm, ctr.y }, plus_col, thick);
		dl->AddLine({ ctr.x, ctr.y - arm }, { ctr.x, ctr.y + arm }, plus_col, thick);

		// Hint text below "+"
		{
			const char* hint = xx("Add item");
			float        hint_fs = ImGui::GetFont()->LegacySize * 0.72f;
			hellvec2     hint_sz = ImGui::GetFont()->CalcTextSizeA(hint_fs, FLT_MAX, 0.f, hint);
			dl->AddText(
				ImGui::GetFont(), hint_fs,
				{ sp.x + (slot_size - hint_sz.x) * 0.5f, ep.y - hint_sz.y - 4.f },
				plus_hovered ? hellcolor(180, 140, 230, 200) : hellcolor(130, 100, 180, 130),
				hint
			);
		}

		if (plus_clicked) {
			g_picker_open = true;
			g_picker_category = cat_knife;
			g_picker_model = 0;
			g_picker_skin = -1;
		}

		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();
		});

	// ── Detail panel ──────────────────────────────────────────────────────
	float detail_x = start_x + grid_w + ImGui::GetStyle().WindowPadding.x;
	ImGui::SetCursorPos({ detail_x, start_y });
	hell::child(xx("##Item"), { detail_w, panel_h }, [&] {
		render_slot_detail(g_selected_slot);
		});

	// ── Picker modal ──────────────────────────────────────────────────────
	if (g_picker_open) {
		hellvec2 win_pos = ImGui::GetWindowPos();
		hellvec2 win_size = ImGui::GetWindowSize();
		ImGui::GetWindowDrawList()->AddRectFilled(
			win_pos, win_pos + win_size,
			hellcolor(0, 0, 0, 140)
		);

		float modal_w = 500.f, modal_h = 360.f;
		ImGui::SetCursorPos({
			(win_size.x - modal_w) * 0.5f,
			(win_size.y - modal_h) * 0.5f
			});

		hell::child(xx("##picker_modal"), { modal_w, modal_h }, [&] {
			hell::label(xx("Add Skin"));
			ImGui::Separator();
			render_picker(g_selected_slot);
			});
	}
}