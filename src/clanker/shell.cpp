// src/clanker/shell.cpp
#include <fstream>
#include <iostream>
#include <sstream>

#include "clanker/builtins.h"
#include "clanker/exec_policy_default.h"
#include "clanker/executor.h"
#include "clanker/line_editor.h"
#include "clanker/parser.h"
#include "clanker/shell.h"
#include "clanker/signals.h"
#include "clanker/util.h"

namespace clanker {

namespace {

// Execute a successfully-parsed unit.
// Current staged semantics:
// - pipeline: run it
// - list: run pipelines sequentially; status is that of the last executed
// pipeline
// - empty: run nothing; preserve last_status
int execute_parse_result(Executor& exec, const ParseResult& pr,
                         int last_status) {
   if (pr.result_is_list()) {
      for (const auto& pl : pr.list.pipelines) {
         if (pl.stages.empty()) continue;
         last_status = exec.run_pipeline(pl);
      }
      return last_status;
   }

   // Legacy single-pipeline path
   if (pr.pipeline.stages.empty()) return last_status;
   return exec.run_pipeline(pr.pipeline);
}

} // namespace

Shell::Shell() {
   const auto p = std::filesystem::current_path();
   root_ = p;
   cwd_ = std::filesystem::weakly_canonical(p);
   oldpwd_ = cwd_;
}

int Shell::run() {
   const auto sec = SecurityPolicy::capture_startup_identity();

   install_signal_handlers();

   LineEditor editor;
   Builtins builtins = make_builtins();

   DefaultExecPolicy policy{root_};
   Executor exec{std::move(builtins), policy, &cwd_, &oldpwd_, sec};

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

      buffer.clear();
      last_status = execute_parse_result(exec, pr, last_status);
   }
}

int Shell::run_string(std::string_view script_text) {
   Builtins builtins = make_builtins();

   DefaultExecPolicy policy{root_};
   const auto sec = SecurityPolicy::capture_startup_identity();
   Executor exec{std::move(builtins), policy, &cwd_, &oldpwd_, sec};

   Parser parser;

   int last_status = 0;

   std::string buffer;
   const std::string text(script_text);
   std::istringstream in{text};

   std::string line;
   while (std::getline(in, line)) {
      // Batch: treat newlines as separators, but allow multi-line completion.
      if (!buffer.empty()) buffer.push_back('\n');
      buffer += line;

      const auto pr = parser.parse(buffer);
      if (pr.kind == ParseKind::Incomplete) {
         continue; // keep accumulating
      }
      if (pr.kind == ParseKind::Error) {
         std::cerr << "parse: " << pr.message << '\n';
         return 2;
      }

      buffer.clear();
      last_status = execute_parse_result(exec, pr, last_status);
   }

   if (!buffer.empty()) {
      // In batch mode, EOF with incomplete construct is an error.
      const auto pr = parser.parse(buffer);
      if (pr.kind == ParseKind::Incomplete) {
         std::cerr << "parse: unexpected end of input\n";
         return 2;
      }
      if (pr.kind == ParseKind::Error) {
         std::cerr << "parse: " << pr.message << '\n';
         return 2;
      }
      last_status = execute_parse_result(exec, pr, last_status);
   }

   return last_status;
}

int Shell::run_file(const std::filesystem::path& script_path) {
   std::ifstream f(script_path);
   if (!f) {
      std::cerr << "clanker: cannot open script: " << script_path << '\n';
      return 2;
   }

   std::ostringstream ss;
   ss << f.rdbuf();
   return run_string(ss.str());
}

} // namespace clanker

