// src/clanker/process.h

#pragma once

#include <string>
#include <vector>

namespace clanker {

// Returns a bash-like exit status: 127 not found, 126 not executable, else
// child status.
int run_external_argv(const std::vector<std::string>& argv);

} // namespace clanker

