#include <Windows.h>

#include <cstdio>
#include <cstring>
#include <type_traits>

#include "core/config_io.h"
#include "core/globals.h"

namespace configio {
namespace {

constexpr uint32_t kMagic   = 0x34464752;   // "RGF4"
constexpr uint32_t kVersion = 3;

struct Header {
    uint32_t magic;
    uint32_t version;
    uint32_t size;      // sizeof(cfg::Config) when the slot was written
    uint32_t reserved;
};

// A memcpy of the config is only defensible while the config stays a plain
// aggregate. If someone adds a std::string or a pointer to it, this breaks the
// build here rather than corrupting saved slots at runtime.
static_assert(std::is_trivially_copyable<cfg::Config>::value,
              "cfg::Config must stay trivially copyable for config_io to memcpy it");

char g_dir[MAX_PATH]{};

void Fail(char* error, size_t len, const char* what) {
    if (error && len)
        snprintf(error, len, "%s", what);
}

bool PathFor(int slot, char* out, size_t len) {
    if (slot < 0 || slot >= kSlots)
        return false;
    snprintf(out, len, "%s\\slot%d.cfg", Directory(), slot + 1);
    return true;
}

} // namespace

const char* Directory() {
    if (g_dir[0])
        return g_dir;

    char temp[MAX_PATH]{};
    const DWORD n = GetTempPathA(MAX_PATH, temp);
    if (!n || n > MAX_PATH)
        snprintf(temp, sizeof(temp), ".\\");

    snprintf(g_dir, sizeof(g_dir), "%s4fir\\configs", temp);

    char parent[MAX_PATH]{};
    snprintf(parent, sizeof(parent), "%s4fir", temp);
    CreateDirectoryA(parent, nullptr);
    CreateDirectoryA(g_dir, nullptr);
    return g_dir;
}

bool Exists(int slot) {
    char path[MAX_PATH]{};
    if (!PathFor(slot, path, sizeof(path)))
        return false;
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

bool Save(int slot, char* error, size_t errorLen) {
    char path[MAX_PATH]{};
    if (!PathFor(slot, path, sizeof(path))) {
        Fail(error, errorLen, "bad slot");
        return false;
    }

    FILE* f = nullptr;
    if (fopen_s(&f, path, "wb") != 0 || !f) {
        Fail(error, errorLen, "cannot open the slot for writing");
        return false;
    }

    Header h{ kMagic, kVersion, static_cast<uint32_t>(sizeof(cfg::Config)), 0 };
    bool ok = fwrite(&h, sizeof(h), 1, f) == 1 &&
              fwrite(&cfg::c, sizeof(cfg::Config), 1, f) == 1;
    fclose(f);

    if (!ok) {
        Fail(error, errorLen, "write failed");
        return false;
    }
    Fail(error, errorLen, "");
    return true;
}

bool Load(int slot, char* error, size_t errorLen) {
    char path[MAX_PATH]{};
    if (!PathFor(slot, path, sizeof(path))) {
        Fail(error, errorLen, "bad slot");
        return false;
    }

    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) {
        Fail(error, errorLen, "slot is empty");
        return false;
    }

    Header h{};
    if (fread(&h, sizeof(h), 1, f) != 1) {
        fclose(f);
        Fail(error, errorLen, "slot is truncated");
        return false;
    }

    if (h.magic != kMagic) {
        fclose(f);
        Fail(error, errorLen, "not a config file");
        return false;
    }
    if (h.version != kVersion || h.size != sizeof(cfg::Config)) {
        fclose(f);
        Fail(error, errorLen, "saved by a different build - settings changed shape");
        return false;
    }

    cfg::Config loaded{};
    const bool ok = fread(&loaded, sizeof(loaded), 1, f) == 1;
    fclose(f);

    if (!ok) {
        Fail(error, errorLen, "read failed");
        return false;
    }

    // Live state must not come back from a file: the unload request and the
    // menu's own visibility belong to this session, not to the slot.
    const int menuKey = cfg::c.menuKey;
    cfg::c = loaded;
    cfg::c.unloadQueued = false;
    cfg::c.menuKey = menuKey;

    Fail(error, errorLen, "");
    return true;
}

bool Remove(int slot, char* error, size_t errorLen) {
    char path[MAX_PATH]{};
    if (!PathFor(slot, path, sizeof(path))) {
        Fail(error, errorLen, "bad slot");
        return false;
    }
    if (!DeleteFileA(path)) {
        Fail(error, errorLen, "slot is empty");
        return false;
    }
    Fail(error, errorLen, "");
    return true;
}

void ResetToDefaults() {
    const int menuKey = cfg::c.menuKey;
    cfg::c = cfg::Config{};
    cfg::c.menuKey = menuKey;
    cfg::c.bots.color = ImVec4(1.f, 0.85f, 0.30f, 1.f);
}

} // namespace configio
