#include <cstdio>

#include "core/config_io.h"
#include "core/globals.h"
#include "ui/skeet.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "CONFIGS" };

int  g_slot = 0;
char g_status[160] = "idle";
bool g_statusIsError = false;

void SetStatus(bool ok, const char* verb, const char* detail) {
    g_statusIsError = !ok;
    if (ok)
        snprintf(g_status, sizeof(g_status), "%s slot %d", verb, g_slot + 1);
    else
        snprintf(g_status, sizeof(g_status), "%s failed: %s", verb,
                 detail && detail[0] ? detail : "unknown");
}

void DrawConfigs() {
    const float w = skeet::ColumnWidth(2);

    // Slot names are built once per frame; six is few enough that this is
    // cheaper than caching it and getting the invalidation wrong.
    static char labels[configio::kSlots][24];
    static const char* labelPtrs[configio::kSlots];
    for (int i = 0; i < configio::kSlots; ++i) {
        snprintf(labels[i], sizeof(labels[i]), "Slot %d%s", i + 1,
                 configio::Exists(i) ? "" : " (empty)");
        labelPtrs[i] = labels[i];
    }

    skeet::BeginGroupBox("Slots", ImVec2(w, 0.f));
    skeet::Combo("Config", &g_slot, labelPtrs, configio::kSlots);

    char error[128] = {};
    if (skeet::Button("Save"))
        SetStatus(configio::Save(g_slot, error, sizeof(error)), "saved", error);
    if (skeet::Button("Load"))
        SetStatus(configio::Load(g_slot, error, sizeof(error)), "loaded", error);
    if (skeet::Button("Delete"))
        SetStatus(configio::Remove(g_slot, error, sizeof(error)), "deleted", error);

    skeet::Separator();
    if (skeet::Button("Reset everything to defaults")) {
        configio::ResetToDefaults();
        g_statusIsError = false;
        snprintf(g_status, sizeof(g_status), "reset to defaults");
    }
    skeet::EndGroupBox();

    ImGui::SameLine();

    skeet::BeginGroupBox("Status", ImVec2(w, 0.f));
    skeet::TextColored(g_statusIsError ? skeet::col::kBad : skeet::col::kGood, "%s",
                       g_status);
    skeet::Separator();
    skeet::TextDim("Files live in:");
    skeet::TextDim("%s", configio::Directory());
    skeet::Separator();
    skeet::TextDim("A slot stores the whole settings struct with");
    skeet::TextDim("its layout version. Rebuild the module with");
    skeet::TextDim("different settings and old slots are refused");
    skeet::TextDim("rather than read as garbage.");
    skeet::Separator();
    skeet::TextDim("The menu key is kept out of a load, so a");
    skeet::TextDim("config can never take the menu away from you.");
    skeet::EndGroupBox();
}

} // namespace

SubTabs ConfigSubs() {
    return { kSubs, IM_ARRAYSIZE(kSubs) };
}

void ConfigDraw(int sub) {
    (void)sub;
    DrawConfigs();
}

} // namespace ui
