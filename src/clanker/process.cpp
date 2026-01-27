// src/clanker/process.cpp

#include <cerrno>
#include <span>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "clanker/process.h"

extern char** environ;

namespace clanker {

namespace {

int status_to_exit_code(int status) {
   if (WIFEXITED(status)) return WEXITSTATUS(status);
   if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
   return 1;
}

std::vector<char*> to_cargv(const std::vector<std::string>& argv) {
   std::vector<char*> out;
   out.reserve(argv.size() + 1);
   for (const auto& s : argv) out.push_back(const_cast<char*>(s.c_str()));
   out.push_back(nullptr);
   return out;
}

} // namespace

int spawn_external(const std::vector<std::string>& argv, int stdin_fd,
                   int stdout_fd, std::span<const int> close_fds) {
   if (argv.empty()) return -EINVAL;

   posix_spawn_file_actions_t actions;
   posix_spawn_file_actions_init(&actions);

   // Close requested fds in the child (before exec).
   for (const int fd : close_fds) {
      if (fd >= 0) posix_spawn_file_actions_addclose(&actions, fd);
   }

   if (stdin_fd != -1)
      posix_spawn_file_actions_adddup2(&actions, stdin_fd, STDIN_FILENO);
   if (stdout_fd != -1)
      posix_spawn_file_actions_adddup2(&actions, stdout_fd, STDOUT_FILENO);

   auto cargv = to_cargv(argv);

   pid_t pid{};
   const int rc =
      posix_spawnp(&pid, cargv[0], &actions, nullptr, cargv.data(), environ);

   posix_spawn_file_actions_destroy(&actions);

   if (rc != 0) {
      // Return negative errno-like value.
      return -rc;
   }

   return static_cast<int>(pid);
}

int run_external_pipeline(const std::vector<std::vector<std::string>>& stages) {
   if (stages.empty()) return 0;

   const std::size_t n = stages.size();
   std::vector<pid_t> pids;
   pids.reserve(n);

   int prev_read = -1;

   for (std::size_t i = 0; i < n; ++i) {
      int pipefd[2] = {-1, -1};
      const bool last = (i + 1 == n);

      if (!last) {
         if (::pipe(pipefd) != 0) {
            if (prev_read != -1) ::close(prev_read);
            return 1;
         }
      }

      const int in_fd = prev_read;
      const int out_fd = last ? -1 : pipefd[1];

      const int pid_or_err =
         spawn_external(stages[i], in_fd, out_fd, std::span<const int>{});

      if (out_fd != -1) ::close(out_fd);
      if (in_fd != -1) ::close(in_fd);

      if (pid_or_err < 0) {
         if (!last) ::close(pipefd[0]);

         const int err = -pid_or_err;
         if (err == ENOENT) return 127;
         return 126;
      }

      pids.push_back(static_cast<pid_t>(pid_or_err));
      prev_read = last ? -1 : pipefd[0];
   }

   if (prev_read != -1) ::close(prev_read);

   int last_status = 0;
   for (std::size_t i = 0; i < pids.size(); ++i) {
      int status = 0;
      for (;;) {
         const pid_t w = ::waitpid(pids[i], &status, 0);
         if (w == -1 && errno == EINTR) continue;
         if (w == -1) status = 1;
         break;
      }
      if (i + 1 == pids.size()) last_status = status_to_exit_code(status);
   }

   return last_status;
}

} // namespace clanker

