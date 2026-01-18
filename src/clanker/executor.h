// src/clanker/executor.h

#pragma once

#include <filesystem>

#include "clanker/ast.h"
#include "clanker/builtins.h"

namespace clanker {

class Executor {
 public:
   Executor(Builtins builtins, std::filesystem::path root);

   // Execute a parsed pipeline (one or more stages).
   // Returns bash-like exit status: status of last stage (pipefail not
   // implemented).
   int run_pipeline(const Pipeline& pipeline);

 private:
   // Execute a single stage (builtin or external) with default stdio.
   int run_simple(const SimpleCommand& cmd);

   // Execute external pipeline stages; first stage reads from stdin_fd.
   // Returns exit status of the last external stage.
   int run_external_pipeline_with_stdin(
      int stdin_fd, const std::vector<std::vector<std::string>>& stages);

   // For "builtin first stage" pipelines: spawn external stages first (reading
   // from pipe), then run builtin writing to pipe, then wait for external
   // stages.
   int run_pipeline_builtin_first(const SimpleCommand& first,
                                  const Pipeline& pipeline);

   Builtins builtins_;
   std::filesystem::path root_;
};

} // namespace clanker

