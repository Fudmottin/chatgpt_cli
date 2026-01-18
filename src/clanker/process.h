// src/clanker/process.h
#pragma once

#include <string>
#include <vector>

namespace clanker {

// Spawn a single external program; connect stdin/stdout via fds.
// Use -1 to mean "inherit".
int spawn_external(const std::vector<std::string>& argv, int stdin_fd,
                   int stdout_fd);

// Run a pipeline of external programs.
// Returns bash-like exit status of the last stage.
int run_external_pipeline(const std::vector<std::vector<std::string>>& stages);

} // namespace clanker

