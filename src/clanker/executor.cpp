// src/clanker/executor.cpp

#include "clanker/executor.h"

#include "clanker/process.h"
#include "clanker/util.h"

#include <iostream>

namespace clanker {

Executor::Executor(Builtins builtins, std::filesystem::path root)
    : builtins_(std::move(builtins)), root_(std::move(root)) {}

int Executor::run(const std::string& command) {
   // Temporary execution model:
   // - split into words (very naive, no quoting)
   // - if argv[0] is a builtin: run it
   // - else: delegate to system()
   //
   // This will be replaced once lexer/parser/AST is implemented.

   const auto argv = split_ws(trim(command));
   if (argv.empty())
      return 0;

   if (auto fn = builtins_.find(argv.front())) {
      BuiltinContext ctx{.root = root_};
      return (*fn)(ctx, argv);
   }

   return run_external_argv(argv);
}

} // namespace clanker

