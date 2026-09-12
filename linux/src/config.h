#pragma once
#include <string>
#include <vector>

// Weapon -> fake skin mapping, loaded from a plain text file and hot-reloaded
// while the game runs (so skins can be changed without re-injecting).
namespace config {

struct SkinEntry {
    int         defIndex  = 0;     // item definition index, e.g. 7 = ak47
    int         paintKit  = 0;
    int         seed      = 0;
    float       wear      = 0.f;
    int         statTrak  = -1;    // -1 = not StatTrak
    std::string nameTag;           // empty = no custom name
};

// Default path: $XDG_CONFIG_HOME/cs2_linux_skinchanger/skins.cfg, falling
// back to $HOME/.config/... . See linux/README.md for the file format.
std::string DefaultPath();

// Re-reads the file if its mtime changed since the last call. Cheap to call
// every tick; does nothing when the file has not changed.
void PollReload(const std::string& path);

const SkinEntry* Find(int defIndex);

} // namespace config
