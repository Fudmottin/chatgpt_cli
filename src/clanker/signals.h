// src/clanker/signals.h
#pragma once

namespace clanker {

void install_signal_handlers();
bool consume_sigint_flag();

// Reap any exited child processes without blocking.
// Safe to call frequently (e.g., each prompt iteration).
void reap_children_nonblocking() noexcept;

} // namespace clanker

