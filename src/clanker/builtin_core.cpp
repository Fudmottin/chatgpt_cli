// src/clanker/builtin_core.cpp

#include "clanker/builtins.h"
#include "clanker/util.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace clanker {
namespace {

int write_line(int fd, std::string_view s) {
   std::string line;
   line.reserve(s.size() + 1);
   line.append(s);
   line.push_back('\n');
   return fd_write_all(fd, line) ? 0 : 1;
}

int write_err(int fd, std::string_view s) { return write_line(fd, s); }

static bool within_root(const std::filesystem::path& root,
                        const std::filesystem::path& p) {
   const auto cr = std::filesystem::weakly_canonical(root);
   const auto cp = std::filesystem::weakly_canonical(p);

   auto r_it = cr.begin();
   auto p_it = cp.begin();
   for (; r_it != cr.end() && p_it != cp.end(); ++r_it, ++p_it) {
      if (*r_it != *p_it)
         return false;
   }
   return r_it == cr.end();
}

Builtins* g_for_help = nullptr;

} // namespace

static int bi_exit(const BuiltinContext&, const Argv& argv) {
   int code = 0;
   if (argv.size() >= 2)
      code = to_int(argv[1]).value_or(0);
   std::exit(code);
}

static int bi_pwd(const BuiltinContext& ctx, const Argv&) {
   const auto s = std::filesystem::current_path().string();
   return write_line(ctx.out_fd, s);
}

static int bi_cd(const BuiltinContext& ctx, const Argv& argv) {
   const auto target = (argv.size() >= 2) ? argv[1] : std::string{"."};

   std::filesystem::path dest = target;
   if (dest.is_relative())
      dest = std::filesystem::current_path() / dest;

   dest = std::filesystem::weakly_canonical(dest);

   if (!within_root(ctx.root, dest)) {
      write_err(ctx.err_fd, "cd: blocked (outside root)");
      return 1;
   }

   std::error_code ec;
   std::filesystem::current_path(dest, ec);
   if (ec) {
      std::string msg = "cd: ";
      msg += ec.message();
      write_err(ctx.err_fd, msg);
      return 1;
   }
   return 0;
}

static int bi_help(const BuiltinContext& ctx, const Argv&) {
   if (!g_for_help)
      return 1;

   for (const auto& [name, help] : g_for_help->help_items()) {
      std::string line;
      line.reserve(name.size() + 2 + help.size());
      line += name;
      line += "  ";
      line += help;
      write_line(ctx.out_fd, line);
   }
   return 0;
}

void add_core_builtins(Builtins& b) {
   b.add("exit", bi_exit, "exit [n] — exit the shell");
   b.add("pwd", bi_pwd, "pwd — print current directory");
   b.add("cd", bi_cd, "cd [dir] — change directory (restricted to root)");
   b.add("help", bi_help, "help — list built-ins");
}

void set_help_registry(Builtins& b) { g_for_help = &b; }

} // namespace clanker

