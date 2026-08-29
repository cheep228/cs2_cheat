#include <cstdio>

#include "core/config_io.h"
#include "core/globals.h"
#include "ui/theme.h"
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
    const float w = theme::ColumnWidth(2);

    // Slot names are built once per frame; six is few enough that this is
    // cheaper than caching it and getting the invalidation wrong.
    static char labels[configio::kSlots][24];
    static const char* labelPtrs[configio::kSlots];
    for (int i = 0; i < configio::kSlots; ++i) {
        snprintf(labels[i], sizeof(labels[i]), "Slot %d%s", i + 1,
                 configio::Exists(i) ? "" : " (empty)");
        labelPtrs[i] = labels[i];
    }

    theme::BeginGroupBox("Slots", ImVec2(w, 0.f));
    theme::Combo("Config", &g_slot, labelPtrs, configio::kSlots);

    char error[128] = {};
    if (theme::Button("Save"))
        SetStatus(configio::Save(g_slot, error, sizeof(error)), "saved", error);
    if (theme::Button("Load"))
        SetStatus(configio::Load(g_slot, error, sizeof(error)), "loaded", error);
    if (theme::Button("Delete"))
        SetStatus(configio::Remove(g_slot, error, sizeof(error)), "deleted", error);

    theme::Separator();
    if (theme::Button("Reset everything to defaults")) {
        configio::ResetToDefaults();
        g_statusIsError = false;
        snprintf(g_status, sizeof(g_status), "reset to defaults");
    }
    theme::EndGroupBox();

    ImGui::SameLine();

    theme::BeginGroupBox("Status", ImVec2(w, 0.f));
    theme::TextColored(g_statusIsError ? theme::col::kBad : theme::col::kGood, "%s",
                       g_status);
    theme::Separator();
    theme::TextDim("Files live in:");
    theme::TextDim("%s", configio::Directory());
    theme::Separator();
    theme::TextDim("A slot stores the whole settings struct with");
    theme::TextDim("its layout version. Rebuild the module with");
    theme::TextDim("different settings and old slots are refused");
    theme::TextDim("rather than read as garbage.");
    theme::Separator();
    theme::TextDim("The menu key is kept out of a load, so a");
    theme::TextDim("config can never take the menu away from you.");
    theme::EndGroupBox();
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
