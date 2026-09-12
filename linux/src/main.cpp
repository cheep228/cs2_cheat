#include <atomic>
#include <chrono>
#include <thread>
#include <unistd.h>

#include "log.h"
#include "skinchanger.h"

namespace {

std::atomic<bool> g_running{false};
std::thread       g_worker;

void WorkerMain() {
    DBG_I("core", "worker thread started (pid %d)", static_cast<int>(getpid()));

    // CS2 loads libclient.so some time after the process starts; Tick()
    // itself is cheap to call before that (EnsureReady() just keeps failing
    // the module lookup), so there is no separate wait loop here.
    while (g_running.load(std::memory_order_relaxed)) {
        skinchanger::Tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    DBG_I("core", "worker thread stopping");
}

} // namespace

extern "C" {

__attribute__((constructor))
static void OnLoad() {
    DBG_I("core", "cs2_linux_skinchanger loaded into pid %d", static_cast<int>(getpid()));
    g_running.store(true, std::memory_order_relaxed);
    g_worker = std::thread(WorkerMain);
}

__attribute__((destructor))
static void OnUnload() {
    g_running.store(false, std::memory_order_relaxed);
    if (g_worker.joinable())
        g_worker.join();
}

} // extern "C"
