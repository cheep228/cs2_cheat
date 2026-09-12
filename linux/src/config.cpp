#include "config.h"

#include <cstdlib>
#include <fstream>
#include <sys/stat.h>

#include "log.h"

namespace config {
namespace {

std::vector<SkinEntry> g_entries;
time_t g_lastMtime = 0;
bool   g_everLoaded = false;

// Parses "key=value" tokens on one line, where value may be a bare word or a
// "quoted string". Line format: "<defindex>: key=value key=value ..."
bool ParseLine(const std::string& line, SkinEntry& out) {
    const size_t colon = line.find(':');
    if (colon == std::string::npos)
        return false;

    out.defIndex = std::atoi(line.substr(0, colon).c_str());
    if (out.defIndex <= 0)
        return false;

    size_t i = colon + 1;
    while (i < line.size()) {
        while (i < line.size() && line[i] == ' ') ++i;
        if (i >= line.size()) break;

        const size_t eq = line.find('=', i);
        if (eq == std::string::npos) break;
        const std::string key = line.substr(i, eq - i);

        size_t vstart = eq + 1;
        std::string value;
        if (vstart < line.size() && line[vstart] == '"') {
            const size_t end = line.find('"', vstart + 1);
            if (end == std::string::npos) break;
            value = line.substr(vstart + 1, end - vstart - 1);
            i = end + 1;
        } else {
            size_t end = line.find(' ', vstart);
            if (end == std::string::npos) end = line.size();
            value = line.substr(vstart, end - vstart);
            i = end;
        }

        if (key == "paintkit")      out.paintKit = std::atoi(value.c_str());
        else if (key == "seed")     out.seed = std::atoi(value.c_str());
        else if (key == "wear")     out.wear = static_cast<float>(std::atof(value.c_str()));
        else if (key == "stattrak") out.statTrak = std::atoi(value.c_str());
        else if (key == "name")     out.nameTag = value;
    }
    return true;
}

void Load(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        if (!g_everLoaded)
            DBG_W("config", "no config at %s yet - skin changer stays idle", path.c_str());
        return;
    }

    std::vector<SkinEntry> parsed;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        SkinEntry e;
        if (ParseLine(line, e))
            parsed.push_back(e);
        else
            DBG_W("config", "could not parse line: %s", line.c_str());
    }

    g_entries = std::move(parsed);
    g_everLoaded = true;
    DBG_OK("config", "loaded %zu skin entries from %s", g_entries.size(), path.c_str());
}

} // namespace

std::string DefaultPath() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    const char* home = std::getenv("HOME");

    std::string base;
    if (xdg && *xdg)
        base = xdg;
    else if (home && *home)
        base = std::string(home) + "/.config";
    else
        base = ".";

    return base + "/cs2_linux_skinchanger/skins.cfg";
}

void PollReload(const std::string& path) {
    struct stat st{};
    if (stat(path.c_str(), &st) != 0)
        return;

    if (st.st_mtime != g_lastMtime) {
        g_lastMtime = st.st_mtime;
        Load(path);
    }
}

const SkinEntry* Find(int defIndex) {
    for (const SkinEntry& e : g_entries) {
        if (e.defIndex == defIndex)
            return &e;
    }
    return nullptr;
}

} // namespace config
