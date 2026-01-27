// src/clanker/exec_policy_default.h
#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <utility>

#include "clanker/exec_policy.h"
#include "clanker/process.h"

namespace clanker {

class DefaultExecPolicy final : public ExecPolicy {
 public:
   explicit DefaultExecPolicy(std::filesystem::path root)
      : root_(std::move(root)) {}

   bool allow_external(std::span<const std::string>,
                       std::string&) const override {
      return true;
   }

   SpawnResult spawn_external(const SpawnSpec& spec) const override {
      const int pid_or_err =
         clanker::spawn_external(spec.argv, spec.stdin_fd, spec.stdout_fd,
                                 spec.stderr_fd, spec.close_fds);
      return SpawnResult{.pid_or_err = pid_or_err};
   }

   const std::filesystem::path& root() const noexcept override { return root_; }

 private:
   std::filesystem::path root_;
};

} // namespace clanker

