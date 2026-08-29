#include "core/settings.h"

#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include "core/globals.h"
#include "core/log.h"

namespace settings {
namespace {

// The loader extracts the payload into this same folder, so the DLL and the
// settings that configure it always travel together.
std::wstring IniPath() {
    wchar_t temp[MAX_PATH]{};
    const DWORD n = GetTempPathW(MAX_PATH, temp);
    if (!n || n > MAX_PATH)
        return {};
    return std::wstring(temp) + L"4fir\\loader_settings.ini";
}

void Trim(std::string& s) {
    const char* ws = " \t\r\n";
    const size_t first = s.find_first_not_of(ws);
    if (first == std::string::npos) {
        s.clear();
        return;
    }
    s = s.substr(first, s.find_last_not_of(ws) - first + 1);
}

// Accepts decimal (what the loader writes) and 0x-prefixed hex, so a
// hand-edited ini behaves the way anyone would expect. Virtual-key codes
// outside 0x01..0xFE are rejected rather than silently binding to nothing.
bool ParseVk(const std::string& value, int& out) {
    char* end = nullptr;
    const long v = strtol(value.c_str(), &end, 0);
    if (end == value.c_str() || v < 0x01 || v > 0xFE)
        return false;
    out = static_cast<int>(v);
    return true;
}

void Apply(const std::string& key, const std::string& value) {
    if (key == "menu_key") {
        int vk = 0;
        if (!ParseVk(value, vk)) {
            DBG_W("settings", "ignoring bad menu_key value '%s'", value.c_str());
            return;
        }
        cfg::c.menuKey = vk;
        DBG_OK("settings", "menu key from loader: 0x%02X", vk);
        return;
    }

    DBG_T("settings", "unknown key '%s' - ignored", key.c_str());
}

} // namespace

bool Load() {
    const std::wstring path = IniPath();
    if (path.empty())
        return false;

    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"r") != 0 || !f) {
        DBG_I("settings", "no loader settings - keeping the baked defaults");
        return false;
    }
    DBG_I("settings", "reading %ls", path.c_str());

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        std::string text(line);

        const size_t comment = text.find_first_of(";#");
        if (comment != std::string::npos)
            text.erase(comment);

        const size_t eq = text.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = text.substr(0, eq);
        std::string value = text.substr(eq + 1);
        Trim(key);
        Trim(value);
        if (!key.empty())
            Apply(key, value);
    }

    fclose(f);
    return true;
}

} // namespace settings
