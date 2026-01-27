// src/clanker/process.h
#pragma once

#include <string>
#include <vector>

namespace clanker {

// Spawn a single external program; connect stdin/stdout/stderr via fds.
// Use -1 to mean "inherit".
// close_fds are forcibly closed in the child before exec (critical for
// pipelines).
int spawn_external(const std::vector<std::string>& argv, int stdin_fd,
                   int stdout_fd, int stderr_fd,
                   const std::vector<int>& close_fds);

// Run a pipeline of external programs (stdin inherited).
// Returns exit status of the last stage.
int run_external_pipeline(const std::vector<std::vector<std::string>>& stages);

} // namespace clanker

