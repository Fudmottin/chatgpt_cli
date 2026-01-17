// src/clanker/executor.h

#pragma once

#include "clanker/builtins.h"

#include <filesystem>
#include <string>

namespace clanker {

class Executor {
public:
   Executor(Builtins builtins, std::filesystem::path root);

   int run(const std::string& command);

private:
   Builtins builtins_;
   std::filesystem::path root_;
};

} // namespace clanker

