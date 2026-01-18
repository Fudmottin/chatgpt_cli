// src/clanker/builtin_core.cpp

#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>

#include "clanker/builtins.h"
#include "clanker/util.h"

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

std::filesystem::path canon(const std::filesystem::path& p) {
   // weakly_canonical tolerates non-existent paths better than canonical().
   return std::filesystem::weakly_canonical(p);
}

bool within_root(const std::filesystem::path& root,
                 const std::filesystem::path& p) {
   const auto cr = canon(root);
   const auto cp = canon(p);

   auto r_it = cr.begin();
   auto p_it = cp.begin();
   for (; r_it != cr.end() && p_it != cp.end(); ++r_it, ++p_it) {
      if (*r_it != *p_it) return false;
   }
   return r_it == cr.end();
}

std::filesystem::path resolve_cd_target(const BuiltinContext& ctx,
                                        std::string_view arg) {
   // Assumes ctx.cwd is valid.
   const auto& root = ctx.root;
   const auto& cwd = *ctx.cwd;

   // bash-like:
   //   cd          -> "home" (in clanker: root)
   //   cd ~        -> root
   //   cd ~/x      -> root/x
   //   cd -        -> oldpwd
   //   cd <rel>    -> cwd/<rel>
   //   cd <abs>    -> <abs> (but must be within root)
   if (arg.empty()) return root;

   if (arg == "-") {
      if (!ctx.oldpwd || ctx.oldpwd->empty()) return {};
      return *ctx.oldpwd;
   }

   if (!arg.empty() && arg[0] == '~') {
      if (arg.size() == 1) return root;

      if (arg.size() >= 2 && arg[1] == '/') {
         return root / std::filesystem::path{std::string{arg.substr(2)}};
      }

      // We do not implement ~user. Keep it explicit.
      return {};
   }

   std::filesystem::path dest{std::string{arg}};
   if (dest.is_relative()) dest = cwd / dest;
   return dest;
}

std::string root_relative_display(const std::filesystem::path& root,
                                  const std::filesystem::path& cwd) {
   // Always show as an absolute-ish root-relative path:
   //   root        -> "/"
   //   root/sub    -> "/sub"
   std::error_code ec;
   const auto rel = std::filesystem::relative(cwd, root, ec);
   if (ec) return "/";

   if (rel.empty() || rel == ".") return "/";

   // Use generic_string for stable separators.
   return "/" + rel.generic_string();
}

Builtins* g_for_help = nullptr;

} // namespace

static int bi_exit(const BuiltinContext&, const Argv& argv) {
   int code = 0;
   if (argv.size() >= 2) code = to_int(argv[1]).value_or(0);
   std::exit(code);
}

static int bi_pwd(const BuiltinContext& ctx, const Argv& argv) {
   const bool relative =
      (argv.size() >= 2) && (argv[1] == "--relative" || argv[1] == "-r");

   if (!ctx.cwd) {
      write_err(ctx.err_fd, "pwd: internal error (cwd not set)");
      return 2;
   }
   const std::filesystem::path cwd = *ctx.cwd;

   if (relative) {
      return write_line(ctx.out_fd, root_relative_display(ctx.root, cwd));
   }

   return write_line(ctx.out_fd, cwd.string());
}

static int bi_cd(const BuiltinContext& ctx, const Argv& argv) {
   if (!ctx.cwd) {
      write_err(ctx.err_fd, "cd: internal error (cwd not set)");
      return 2;
   }

   std::string_view arg;
   if (argv.size() >= 2) arg = argv[1];

   // cd with no args -> root
   if (argv.size() == 1) arg = std::string_view{};

   if (arg == "-" && (!ctx.oldpwd || ctx.oldpwd->empty())) {
      write_err(ctx.err_fd, "cd: OLDPWD not set");
      return 1;
   }

   const std::filesystem::path raw = resolve_cd_target(ctx, arg);
   if (raw.empty()) {
      write_err(ctx.err_fd, (arg.size() && arg[0] == '~')
                               ? "cd: unsupported ~ form"
                               : "cd: invalid target");
      return 1;
   }

   const auto dest = canon(raw);

   // Enforce root sandbox for cd.
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

   // Update PWD/OLDPWD state.
   if (ctx.oldpwd) *ctx.oldpwd = *ctx.cwd;
   *ctx.cwd = dest;

   // bash prints the new directory for "cd -"
   if (arg == "-") write_line(ctx.out_fd, dest.string());

   return 0;
}

static int bi_help(const BuiltinContext& ctx, const Argv&) {
   if (!g_for_help) return 1;

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
   b.add("pwd", bi_pwd, "pwd [--relative|-r] — print current directory");
   b.add("cd", bi_cd,
         "cd [dir|-|~|~/path] — change directory (restricted to root)");
   b.add("help", bi_help, "help — list built-ins");
}

void set_help_registry(Builtins& b) { g_for_help = &b; }

} // namespace clanker

