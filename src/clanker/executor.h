// src/clanker/executor.h
#pragma once

#include <filesystem>

#include "clanker/ast.h"
#include "clanker/builtins.h"
#include "clanker/exec_policy.h"
#include "clanker/security_policy.h"

namespace clanker {

class Executor {
 public:
   Executor(Builtins builtins, const ExecPolicy& policy,
            std::filesystem::path* cwd, std::filesystem::path* oldpwd,
            SecurityPolicy sec);

   int run_pipeline(const Pipeline& pipeline);

   // TODO 2 execution surface:
   int run_andor(const AndOr& ao);
   int run_list(const CommandList& list);

 private:
   int run_simple(const SimpleCommand& cmd);
   int run_pipeline_builtin_first(const SimpleCommand& first,
                                  const Pipeline& pipeline);
   int run_pipeline_all_external(const Pipeline& pipeline);

   int run_background(const AndOr& ao);

   Builtins builtins_;
   const ExecPolicy& policy_;
   SecurityPolicy sec_;
   std::filesystem::path* cwd_{nullptr};
   std::filesystem::path* oldpwd_{nullptr};
};

} // namespace clanker

