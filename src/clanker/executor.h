// src/clanker/executor.h
#pragma once

#include <filesystem>

#include "clanker/ast.h"
#include "clanker/builtins.h"
#include "clanker/exec_policy.h"

namespace clanker {

class Executor {
 public:
   Executor(Builtins builtins, const ExecPolicy& policy,
            std::filesystem::path* cwd, std::filesystem::path* oldpwd);

   int run_pipeline(const Pipeline& pipeline);

 private:
   int run_simple(const SimpleCommand& cmd);
   int run_pipeline_builtin_first(const SimpleCommand& first,
                                  const Pipeline& pipeline);
   int run_pipeline_all_external(const Pipeline& pipeline);

   Builtins builtins_;
   const ExecPolicy& policy_;
   std::filesystem::path* cwd_{nullptr};
   std::filesystem::path* oldpwd_{nullptr};
};

} // namespace clanker

