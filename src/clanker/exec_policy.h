// src/clanker/exec_policy.h
#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace clanker {

struct SpawnSpec {
   std::vector<std::string> argv;

   // Use -1 to inherit from the parent.
   int stdin_fd = -1;
   int stdout_fd = -1;
   int stderr_fd = -1;

   // FDs to close in the child before exec (pipeline hygiene).
   std::vector<int> close_fds;
};

struct SpawnResult {
   // >= 0: pid
   // <  0: -errno-like (posix_spawn* return code)
   int pid_or_err = -1;
};

class ExecPolicy {
 public:
   virtual ~ExecPolicy() = default;

   // Return false and set reason if this external command is disallowed.
   virtual bool allow_external(std::span<const std::string> argv,
                               std::string& reason) const = 0;

   // Spawn an external process. Policy may rewrite argv/env/paths.
   virtual SpawnResult spawn_external(const SpawnSpec& spec) const = 0;

   virtual const std::filesystem::path& root() const noexcept = 0;
};

} // namespace clanker

