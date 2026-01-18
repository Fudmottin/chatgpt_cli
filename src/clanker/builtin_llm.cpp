// src/clanker/builtin_llm.cpp

#include <cstddef>
#include <string>
#include <string_view>

#include "clanker/builtins.h"
#include "clanker/util.h"

namespace clanker {
namespace {

using namespace std::literals;

int write_line(int fd, std::string_view s) {
   // Always terminate lines consistently for human-facing output.
   std::string line;
   line.reserve(s.size() + 1);
   line.append(s);
   line.push_back('\n');

   return fd_write_all(fd, line) ? 0 : 1;
}

int write_err(int fd, std::string_view s) { return write_line(fd, s); }

std::string join_args(const Argv& argv, std::size_t start) {
   std::string out;
   if (start >= argv.size()) return out;

   // Reserve a lower bound to reduce reallocation; not exact but good enough.
   std::size_t total = 0;
   for (std::size_t i = start; i < argv.size(); ++i)
      total += argv[i].size() + 1;
   out.reserve(total);

   for (std::size_t i = start; i < argv.size(); ++i) {
      if (i != start) out.push_back(' ');
      out += argv[i];
   }
   return out;
}

int print_stub_response(const BuiltinContext& ctx, std::string_view tag,
                        const Argv& argv, std::size_t start) {
   std::string out;
   out.reserve(64);

   out += "[stub ";
   out += tag;
   out += "] ";

   out += join_args(argv, start);
   out.push_back('\n');

   return fd_write_all(ctx.out_fd, out) ? 0 : 1;
}

int require_min_args(const BuiltinContext& ctx, const Argv& argv,
                     std::size_t min_args, std::string_view usage_line) {
   if (argv.size() >= min_args) return 0;

   // Keep errors on stderr, bash-style.
   std::string msg;
   msg.reserve(64);
   msg += std::string{argv.empty() ? "llm"sv : std::string_view{argv[0]}};
   msg += ": ";
   msg += std::string{usage_line};
   return write_err(ctx.err_fd, msg);
}

} // namespace

static int bi_models(const BuiltinContext& ctx, const Argv&) {
   // Keep output stable and machine-friendly.
   // One model per line: "<backend>:<model-id>"
   constexpr std::string_view k = "openai:gpt-stub\n"
                                  "anthropic:claude-stub\n";
   return fd_write_all(ctx.out_fd, k) ? 0 : 1;
}

static int bi_use(const BuiltinContext& ctx, const Argv& argv) {
   if (int rc = require_min_args(ctx, argv, 2, "use <backend> [model=<id>]");
       rc != 0)
      return 2;

   // Stub: no persistent config yet.
   std::string out = "default backend set to: " + argv[1] + " (stub)\n";
   return fd_write_all(ctx.out_fd, out) ? 0 : 1;
}

static int bi_prompt(const BuiltinContext& ctx, const Argv& argv) {
   if (int rc = require_min_args(ctx, argv, 2, "prompt <text...>"); rc != 0)
      return 2;

   // Stub uses implicit default backend.
   return print_stub_response(ctx, "llm", argv, 1);
}

static int bi_ask(const BuiltinContext& ctx, const Argv& argv) {
   if (int rc = require_min_args(ctx, argv, 3, "ask <backend> <text...>");
       rc != 0)
      return 2;

   return print_stub_response(ctx, argv[1], argv, 2);
}

void add_llm_builtins(Builtins& b) {
   b.add("models", bi_models, "models — list configured model backends");
   b.add("use", bi_use,
         "use <backend> [model=<id>] — select default backend (stub)");
   b.add("prompt", bi_prompt,
         "prompt <text...> — send text to default model (stub)");
   b.add("ask", bi_ask,
         "ask <backend> <text...> — send text to backend (stub)");
}

} // namespace clanker

