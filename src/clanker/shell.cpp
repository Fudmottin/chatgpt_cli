#include "clanker/shell.h"

#include "clanker/builtins.h"
#include "clanker/executor.h"
#include "clanker/line_editor.h"
#include "clanker/parser.h"
#include "clanker/signals.h"
#include "clanker/util.h"

#include <iostream>

namespace clanker {

Shell::Shell()
    : root_(std::filesystem::current_path()) {}

int Shell::run() {
   install_signal_handlers();

   LineEditor editor;
   Builtins builtins = make_builtins();
   Executor exec{builtins, root_};
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
      const std::string command = pr.logical_command;
      buffer.clear();

      if (trim(command).empty())
         continue;

      last_status = exec.run(command);
   }
}

} // namespace clanker

