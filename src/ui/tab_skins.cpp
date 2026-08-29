#include "ui/theme.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "WEAPONS" };

void DrawWeapons() {
    const float w = theme::ColumnWidth(1);

    theme::BeginGroupBox("Skin changer", ImVec2(w, 0.f));
    theme::TextColored(theme::col::kBad, "Not implemented - and not faked.");
    theme::Separator();
    theme::TextDim("A skin changer rewrites the econ item attached to your");
    theme::TextDim("weapon entity: paint kit, wear, seed, StatTrak, name tag, and");
    theme::TextDim("then forces the view model to rebuild so the change shows.");
    theme::Separator();
    theme::TextDim("This project has none of the pieces that needs. There is no");
    theme::TextDim("item-schema reader, no CEconItemView layout, and no hook on");
    theme::TextDim("the model rebuild - only the weapon definition index, which");
    theme::TextDim("sdk/weapon.cpp reads to pick ballistics out of a table.");
    theme::Separator();
    theme::TextDim("A dropdown of paint kits here would change nothing in game,");
    theme::TextDim("so there is not one. The honest state of this tab is empty");
    theme::TextDim("until sdk/ grows an econ layer.");
    theme::EndGroupBox();
}

} // namespace

SubTabs SkinsSubs() {
    return { kSubs, IM_ARRAYSIZE(kSubs) };
}

void SkinsDraw(int sub) {
    (void)sub;
    DrawWeapons();
}

} // namespace ui
