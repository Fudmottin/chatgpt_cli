// src/clanker/shell.cpp

#include <iostream>

#include "clanker/builtins.h"
#include "clanker/executor.h"
#include "clanker/line_editor.h"
#include "clanker/parser.h"
#include "clanker/shell.h"
#include "clanker/signals.h"
#include "clanker/util.h"

namespace clanker {

Shell::Shell() {
   const auto p = std::filesystem::current_path();
   root_ = p;
   cwd_ = std::filesystem::weakly_canonical(p);
   oldpwd_ = cwd_;
}

int Shell::run() {
   install_signal_handlers();

   LineEditor editor;
   Builtins builtins = make_builtins();
   Executor exec{make_builtins(), root_, &cwd_, &oldpwd_};
   Parser parser;

   std::string buffer;
   int last_status = 0;

   for (;;) {
      if (consume_sigint_flag()) {
         std::cout << '\n';
         buffer.clear();
      }

      const bool continuing = !buffer.empty();
      const std::string prompt = continuing ? "... " : "clanker > ";

      auto line_opt = editor.readline(prompt);
      if (!line_opt) {
         std::cout << '\n';
         return last_status; // EOF (Ctrl-D)
      }

      std::string line = *line_opt;
      if (buffer.empty())
         buffer = line;
      else
         buffer += '\n' + line;

      const auto pr = parser.parse(buffer);
      if (pr.kind == ParseKind::Incomplete) {
         continue;
      }
      if (pr.kind == ParseKind::Error) {
         std::cerr << "syntax error: " << pr.message << '\n';
         buffer.clear();
         last_status = 2;
         continue;
      }

      // Complete:
      buffer.clear();

      if (pr.pipeline.stages.empty()) continue;

      last_status = exec.run_pipeline(pr.pipeline);
   }
}

} // namespace clanker

