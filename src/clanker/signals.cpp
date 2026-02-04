// src/clanker/signals.cpp

#include <atomic>
#include <cerrno>
#include <csignal>
#include <sys/wait.h>

#include "clanker/signals.h"

namespace clanker {

static std::atomic<bool> g_got_sigint{false};

static void on_sigint(int) {
   g_got_sigint.store(true, std::memory_order_relaxed);
}

void install_signal_handlers() { std::signal(SIGINT, on_sigint); }

bool consume_sigint_flag() {
   return g_got_sigint.exchange(false, std::memory_order_relaxed);
}

void reap_children_nonblocking() noexcept {
   for (;;) {
      int status = 0;
      const pid_t w = ::waitpid(-1, &status, WNOHANG);
      if (w > 0) continue; // reaped one; try for more
      if (w == 0) break;   // none ready
      if (w < 0 && errno == EINTR) continue;
      break; // ECHILD or other: stop
   }
}

} // namespace clanker

