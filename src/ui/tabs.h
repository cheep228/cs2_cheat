#pragma once

// Tab content.
//
// Each main tab lives in its own translation unit and knows nothing about the
// window, the tab strip or the style - it just fills the content region with
// theme:: widgets. menu.cpp owns the chrome and dispatches here.
namespace ui {

struct SubTabs {
    const char* const* names = nullptr;
    int                count = 0;
};

SubTabs LegitSubs();
void    LegitDraw(int sub);

SubTabs VisualsSubs();
void    VisualsDraw(int sub);

SubTabs MiscSubs();
void    MiscDraw(int sub);

SubTabs SkinsSubs();
void    SkinsDraw(int sub);

SubTabs ConfigSubs();
void    ConfigDraw(int sub);

} // namespace ui
