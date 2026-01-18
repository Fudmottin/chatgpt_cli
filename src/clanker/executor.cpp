// src/clanker/executor.cpp

#include <cerrno>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "clanker/executor.h"
#include "clanker/process.h"
#include "clanker/util.h"

namespace clanker {

namespace {

int status_to_exit_code(int status) {
   if (WIFEXITED(status)) return WEXITSTATUS(status);
   if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
   return 1;
}

bool is_builtin(const Builtins& b, const SimpleCommand& st) {
   return !st.argv.empty() && b.find(st.argv.front()).has_value();
}

} // namespace

Executor::Executor(Builtins builtins, std::filesystem::path root,
                   std::filesystem::path* cwd, std::filesystem::path* oldpwd)
   : builtins_(std::move(builtins))
   , root_(std::move(root))
   , cwd_(cwd)
   , oldpwd_(oldpwd) {}

int Executor::run_simple(const SimpleCommand& cmd) {
   if (cmd.argv.empty()) return 0;

   if (auto fn = builtins_.find(cmd.argv.front())) {
      BuiltinContext ctx{.root = root_,
                         .in_fd = STDIN_FILENO,
                         .out_fd = STDOUT_FILENO,
                         .err_fd = STDERR_FILENO,
                         .cwd = cwd_,
                         .oldpwd = oldpwd_};
      return (*fn)(ctx, cmd.argv);
   }

   return run_external_pipeline({cmd.argv});
}

int Executor::run_pipeline_builtin_first(const SimpleCommand& first,
                                         const Pipeline& pipeline) {
   int pipefd[2] = {-1, -1};
   if (::pipe(pipefd) != 0) return 1;

   const int read_end = pipefd[0];
   const int write_end = pipefd[1];

   // External stages: 1..end (must be external for now)
   std::vector<std::vector<std::string>> external;
   external.reserve(pipeline.stages.size() - 1);
   for (std::size_t i = 1; i < pipeline.stages.size(); ++i) {
      const auto& st = pipeline.stages[i];
      if (st.argv.empty()) {
         ::close(read_end);
         ::close(write_end);
         return 2;
      }
      if (builtins_.find(st.argv.front())) {
         ::close(read_end);
         ::close(write_end);
         fd_write_all(STDERR_FILENO, "error: built-ins in non-first pipeline "
                                     "stages not implemented yet\n");
         return 2;
      }
      external.push_back(st.argv);
   }

   // Spawn external pipeline (no wait yet), stdin=read_end.
   std::vector<pid_t> pids;
   pids.reserve(external.size());

   int prev_read = read_end;

   for (std::size_t i = 0; i < external.size(); ++i) {
      int next_pipe[2] = {-1, -1};
      const bool last = (i + 1 == external.size());

      if (!last) {
         if (::pipe(next_pipe) != 0) {
            if (prev_read != -1) ::close(prev_read);
            ::close(write_end);
            return 1;
         }
      }

      const int in_fd = prev_read;
      const int out_fd = last ? -1 : next_pipe[1];

      // Critical: ensure no external child keeps the builtin pipe write end
      // open.
      const int pid_or_err =
         spawn_external(external[i], in_fd, out_fd, {write_end});

      if (out_fd != -1) ::close(out_fd);
      if (in_fd != -1) ::close(in_fd);

      if (pid_or_err < 0) {
         if (!last) ::close(next_pipe[0]);
         ::close(write_end);

         const int err = -pid_or_err;
         if (err == ENOENT) return 127;
         return 126;
      }

      pids.push_back(static_cast<pid_t>(pid_or_err));
      prev_read = last ? -1 : next_pipe[0];
   }

   if (prev_read != -1) ::close(prev_read);

   // Run builtin writing to pipe.
   int builtin_status = 0;
   if (auto fn = builtins_.find(first.argv.front())) {
      BuiltinContext ctx{.root = root_,
                         .in_fd = STDIN_FILENO,
                         .out_fd = write_end,
                         .err_fd = STDERR_FILENO,
                         .cwd = cwd_,
                         .oldpwd = oldpwd_};
      builtin_status = (*fn)(ctx, first.argv);
   } else {
      builtin_status = 2;
   }

   ::close(write_end); // deliver EOF to external pipeline

   // Wait for externals; return last stage status (bash default).
   int last_exit = 0;
   for (std::size_t i = 0; i < pids.size(); ++i) {
      int status = 0;
      for (;;) {
         const pid_t w = ::waitpid(pids[i], &status, 0);
         if (w == -1 && errno == EINTR) continue;
         if (w == -1) status = 1;
         break;
      }
      if (i + 1 == pids.size()) last_exit = status_to_exit_code(status);
   }

   (void)builtin_status; // pipefail not implemented
   return last_exit;
}

int Executor::run_pipeline(const Pipeline& pipeline) {
   if (pipeline.stages.empty()) return 0;

   if (pipeline.stages.size() == 1) return run_simple(pipeline.stages[0]);

   const auto& first = pipeline.stages.front();
   if (is_builtin(builtins_, first))
      return run_pipeline_builtin_first(first, pipeline);

   std::vector<std::vector<std::string>> stages;
   stages.reserve(pipeline.stages.size());
   for (const auto& st : pipeline.stages) {
      if (st.argv.empty()) return 2;
      if (builtins_.find(st.argv.front())) {
         fd_write_all(STDERR_FILENO,
                      "error: built-ins in pipelines not implemented yet\n");
         return 2;
      }
      stages.push_back(st.argv);
   }

   return run_external_pipeline(stages);
}

} // namespace clanker

