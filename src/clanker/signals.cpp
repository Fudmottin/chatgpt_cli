#include "clanker/signals.h"

#include <atomic>
#include <csignal>

namespace clanker {

static std::atomic<bool> g_got_sigint{false};

static void on_sigint(int) {
   g_got_sigint.store(true, std::memory_order_relaxed);
}

void install_signal_handlers() {
   std::signal(SIGINT, on_sigint);
}

bool consume_sigint_flag() {
   return g_got_sigint.exchange(false, std::memory_order_relaxed);
}

} // namespace clanker

