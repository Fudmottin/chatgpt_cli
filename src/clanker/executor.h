// src/clanker/executor.h
#pragma once

#include "clanker/ast.h"
#include "clanker/builtins.h"

#include <filesystem>

namespace clanker {

class Executor {
public:
   Executor(Builtins builtins, std::filesystem::path root);

   int run_pipeline(const Pipeline& pipeline);

private:
   int run_simple(const SimpleCommand& cmd);

   Builtins builtins_;
   std::filesystem::path root_;
};

} // namespace clanker

