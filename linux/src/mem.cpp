#include "mem.h"

#include <atomic>
#include <csetjmp>
#include <csignal>
#include <cstring>

namespace {

thread_local sigjmp_buf   g_jmp;
thread_local volatile sig_atomic_t g_guarded = 0;

struct sigaction g_oldSegv{};
struct sigaction g_oldBus{};
std::atomic<bool> g_installed{false};

// Only faults that happen while we are inside a guarded CopyBytes/WriteBytes
// call are ours to swallow. Anything else - a real game crash on another
// thread, or on this one outside the guard - is handed to whatever handler
// was installed before us (the game's own, or the default), so we never mask
// an unrelated bug.
void Handler(int sig, siginfo_t* info, void* ctx) {
    if (g_guarded) {
        siglongjmp(g_jmp, 1);
        return;
    }

    struct sigaction* old = (sig == SIGSEGV) ? &g_oldSegv : &g_oldBus;
    if ((old->sa_flags & SA_SIGINFO) && old->sa_sigaction) {
        old->sa_sigaction(sig, info, ctx);
        return;
    }
    if (!old->sa_handler || old->sa_handler == SIG_DFL) {
        signal(sig, SIG_DFL);
        raise(sig);
        return;
    }
    if (old->sa_handler != SIG_IGN)
        old->sa_handler(sig);
}

} // namespace

namespace mem {

void InstallGuard() {
    if (g_installed.exchange(true))
        return;

    struct sigaction sa{};
    sa.sa_sigaction = Handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &g_oldSegv);
    sigaction(SIGBUS, &sa, &g_oldBus);
}

bool CopyBytes(uintptr_t address, void* dst, size_t size) {
    InstallGuard();

    if (sigsetjmp(g_jmp, 1)) {
        g_guarded = 0;
        ++faults;
        return false;
    }

    g_guarded = 1;
    std::memcpy(dst, reinterpret_cast<const void*>(address), size);
    g_guarded = 0;
    return true;
}

bool WriteBytes(uintptr_t address, const void* src, size_t size) {
    InstallGuard();

    if (sigsetjmp(g_jmp, 1)) {
        g_guarded = 0;
        ++faults;
        return false;
    }

    g_guarded = 1;
    std::memcpy(reinterpret_cast<void*>(address), src, size);
    g_guarded = 0;
    return true;
}

std::string ReadString(uintptr_t address, size_t maxLen) {
    std::string out;
    if (!Valid(address))
        return out;

    out.resize(maxLen);
    for (size_t i = 0; i < maxLen; ++i) {
        const char c = Read<char>(address + i);
        if (c == '\0') {
            out.resize(i);
            return out;
        }
        out[i] = c;
    }
    return out;
}

bool WriteString(uintptr_t address, const std::string& value, size_t cap) {
    if (!Valid(address) || cap == 0)
        return false;

    const size_t n = value.size() < cap - 1 ? value.size() : cap - 1;
    if (!WriteBytes(address, value.data(), n))
        return false;
    const char nul = '\0';
    return WriteBytes(address + n, &nul, 1);
}

} // namespace mem
