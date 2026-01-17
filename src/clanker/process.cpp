// src/clanker/process.cpp

#include <cerrno>
#include <cstdlib>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "clanker/process.h"

extern char** environ;

namespace clanker {

static int status_to_exit_code(int status) {
   if (WIFEXITED(status)) return WEXITSTATUS(status);
   if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
   return 1;
}

int run_external_argv(const std::vector<std::string>& argv) {
   if (argv.empty()) return 0;

   std::vector<char*> cargv;
   cargv.reserve(argv.size() + 1);
   for (const auto& s : argv) cargv.push_back(const_cast<char*>(s.c_str()));
   cargv.push_back(nullptr);

   pid_t pid{};
   const int rc =
      posix_spawnp(&pid, cargv[0], nullptr, nullptr, cargv.data(), environ);
   if (rc != 0) {
      // posix_spawnp returns an errno value, not -1.
      if (rc == ENOENT) return 127;
      return 126;
   }

   int status = 0;
   for (;;) {
      const pid_t w = waitpid(pid, &status, 0);
      if (w == -1 && errno == EINTR) continue;
      if (w == -1) return 1;
      break;
   }
   return status_to_exit_code(status);
}

} // namespace clanker

