// src/clanker/ast.h
#pragma once

#include <string>
#include <vector>

namespace clanker {

struct SimpleCommand {
   std::vector<std::string> argv;
};

struct Pipeline {
   std::vector<SimpleCommand> stages; // size >= 1
};

} // namespace clanker

