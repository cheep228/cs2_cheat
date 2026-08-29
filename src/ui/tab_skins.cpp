#include "ui/skeet.h"
#include "ui/tabs.h"

namespace ui {
namespace {

const char* const kSubs[] = { "WEAPONS" };

void DrawWeapons() {
    const float w = skeet::ColumnWidth(1);

    skeet::BeginGroupBox("Skin changer", ImVec2(w, 0.f));
    skeet::TextColored(skeet::col::kBad, "Not implemented - and not faked.");
    skeet::Separator();
    skeet::TextDim("A skin changer rewrites the econ item attached to your");
    skeet::TextDim("weapon entity: paint kit, wear, seed, StatTrak, name tag, and");
    skeet::TextDim("then forces the view model to rebuild so the change shows.");
    skeet::Separator();
    skeet::TextDim("This project has none of the pieces that needs. There is no");
    skeet::TextDim("item-schema reader, no CEconItemView layout, and no hook on");
    skeet::TextDim("the model rebuild - only the weapon definition index, which");
    skeet::TextDim("sdk/weapon.cpp reads to pick ballistics out of a table.");
    skeet::Separator();
    skeet::TextDim("A dropdown of paint kits here would change nothing in game,");
    skeet::TextDim("so there is not one. The honest state of this tab is empty");
    skeet::TextDim("until sdk/ grows an econ layer.");
    skeet::EndGroupBox();
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
