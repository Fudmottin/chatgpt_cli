// src/clanker/executor.cpp
#include <cerrno>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "clanker/executor.h"
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

class UniqueFd {
 public:
   UniqueFd() = default;
   explicit UniqueFd(int fd)
      : fd_(fd) {}
   ~UniqueFd() { reset(); }

   UniqueFd(const UniqueFd&) = delete;
   UniqueFd& operator=(const UniqueFd&) = delete;

   UniqueFd(UniqueFd&& o) noexcept
      : fd_(o.fd_) {
      o.fd_ = -1;
   }
   UniqueFd& operator=(UniqueFd&& o) noexcept {
      if (this != &o) {
         reset();
         fd_ = o.fd_;
         o.fd_ = -1;
      }
      return *this;
   }

   int get() const noexcept { return fd_; }
   int release() noexcept {
      const int out = fd_;
      fd_ = -1;
      return out;
   }

   void reset(int fd = -1) noexcept {
      if (fd_ != -1) ::close(fd_);
      fd_ = fd;
   }

 private:
   int fd_{-1};
};

int make_pipe(UniqueFd& r, UniqueFd& w) {
   int fds[2] = {-1, -1};
   if (::pipe(fds) != 0) return errno ? errno : 1;
   r.reset(fds[0]);
   w.reset(fds[1]);
   return 0;
}

} // namespace

Executor::Executor(Builtins builtins, const ExecPolicy& policy,
                   std::filesystem::path* cwd, std::filesystem::path* oldpwd)
   : builtins_(std::move(builtins))
   , policy_(policy)
   , cwd_(cwd)
   , oldpwd_(oldpwd) {}

int Executor::run_simple(const SimpleCommand& cmd) {
   if (cmd.argv.empty()) return 0;

   if (auto fn = builtins_.find(cmd.argv.front())) {
      BuiltinContext ctx{.root = policy_.root(),
                         .in_fd = STDIN_FILENO,
                         .out_fd = STDOUT_FILENO,
                         .err_fd = STDERR_FILENO,
                         .cwd = cwd_,
                         .oldpwd = oldpwd_};
      return (*fn)(ctx, cmd.argv);
   }

   std::string reason;
   if (!policy_.allow_external(cmd.argv, reason)) {
      if (reason.empty()) reason = "disallowed by policy";
      fd_write_all(STDERR_FILENO, "error: " + reason + "\n");
      return 126;
   }

   SpawnSpec spec;
   spec.argv = cmd.argv;
   spec.stdin_fd = -1;
   spec.stdout_fd = -1;
   spec.stderr_fd = -1;

   const auto r = policy_.spawn_external(spec);
   if (r.pid_or_err < 0) {
      const int err = -r.pid_or_err;
      if (err == ENOENT) return 127;
      return 126;
   }

   int status = 0;
   for (;;) {
      const pid_t w = ::waitpid(static_cast<pid_t>(r.pid_or_err), &status, 0);
      if (w == -1 && errno == EINTR) continue;
      if (w == -1) status = 1;
      break;
   }
   return status_to_exit_code(status);
}

int Executor::run_pipeline_builtin_first(const SimpleCommand& first,
                                         const Pipeline& pipeline) {
   // Pipe from builtin stdout -> stdin of the external pipeline.
   UniqueFd read_end;
   UniqueFd write_end;
   if (int ec = make_pipe(read_end, write_end); ec != 0) return 1;

   // Spawn external stages: 1..end (must be external for now).
   std::vector<pid_t> pids;
   pids.reserve(pipeline.stages.size() - 1);

   UniqueFd prev_read(read_end.release());

   for (std::size_t i = 1; i < pipeline.stages.size(); ++i) {
      const auto& st = pipeline.stages[i];
      if (st.argv.empty()) return 2;

      if (builtins_.find(st.argv.front())) {
         fd_write_all(STDERR_FILENO,
                      "error: built-ins in non-first pipeline stages "
                      "not implemented yet\n");
         return 2;
      }

      std::string reason;
      if (!policy_.allow_external(st.argv, reason)) {
         if (reason.empty()) reason = "disallowed by policy";
         fd_write_all(STDERR_FILENO, "error: " + reason + "\n");
         return 126;
      }

      UniqueFd next_read;
      UniqueFd next_write;
      const bool last = (i + 1 == pipeline.stages.size());

      if (!last) {
         if (int ec = make_pipe(next_read, next_write); ec != 0) return 1;
      }

      SpawnSpec spec;
      spec.argv = st.argv;
      spec.stdin_fd = prev_read.get();
      spec.stdout_fd = last ? -1 : next_write.get();
      spec.stderr_fd = -1;

      // Critical: ensure the external child does not keep the builtin write end
      // open (otherwise EOF never reaches it).
      spec.close_fds.push_back(write_end.get());

      const auto r = policy_.spawn_external(spec);

      if (r.pid_or_err < 0) {
         const int err = -r.pid_or_err;
         if (err == ENOENT) return 127;
         return 126;
      }

      pids.push_back(static_cast<pid_t>(r.pid_or_err));

      // Parent closes what it no longer needs.
      prev_read.reset();
      if (!last) next_write.reset();
      prev_read = std::move(next_read);
   }

   // Run builtin writing to pipe.
   int builtin_status = 0;
   if (auto fn = builtins_.find(first.argv.front())) {
      BuiltinContext ctx{.root = policy_.root(),
                         .in_fd = STDIN_FILENO,
                         .out_fd = write_end.get(),
                         .err_fd = STDERR_FILENO,
                         .cwd = cwd_,
                         .oldpwd = oldpwd_};
      builtin_status = (*fn)(ctx, first.argv);
   } else {
      builtin_status = 2;
   }

   (void)builtin_status; // pipefail not implemented

   // Close write end to deliver EOF to the external pipeline.
   write_end.reset();

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

   return last_exit;
}

int Executor::run_pipeline_all_external(const Pipeline& pipeline) {
   if (pipeline.stages.empty()) return 0;

   // Validate and policy-check first (fail fast, no partial execution).
   for (const auto& st : pipeline.stages) {
      if (st.argv.empty()) return 2;
      if (builtins_.find(st.argv.front())) {
         fd_write_all(STDERR_FILENO,
                      "error: built-ins in pipelines not implemented yet\n");
         return 2;
      }
      std::string reason;
      if (!policy_.allow_external(st.argv, reason)) {
         if (reason.empty()) reason = "disallowed by policy";
         fd_write_all(STDERR_FILENO, "error: " + reason + "\n");
         return 126;
      }
   }

   std::vector<pid_t> pids;
   pids.reserve(pipeline.stages.size());

   UniqueFd prev_read;

   for (std::size_t i = 0; i < pipeline.stages.size(); ++i) {
      const bool last = (i + 1 == pipeline.stages.size());

      UniqueFd next_read;
      UniqueFd next_write;
      if (!last) {
         if (int ec = make_pipe(next_read, next_write); ec != 0) return 1;
      }

      SpawnSpec spec;
      spec.argv = pipeline.stages[i].argv;
      spec.stdin_fd = prev_read.get();
      spec.stdout_fd = last ? -1 : next_write.get();
      spec.stderr_fd = -1;

      const auto r = policy_.spawn_external(spec);

      if (r.pid_or_err < 0) {
         const int err = -r.pid_or_err;
         if (err == ENOENT) return 127;
         return 126;
      }

      pids.push_back(static_cast<pid_t>(r.pid_or_err));

      // Parent closes what it no longer needs.
      prev_read.reset();
      if (!last) next_write.reset();
      prev_read = std::move(next_read);
   }

   // Close last read end (if any) in parent.
   prev_read.reset();

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

int Executor::run_pipeline(const Pipeline& pipeline) {
   if (pipeline.stages.empty()) return 0;
   if (pipeline.stages.size() == 1) return run_simple(pipeline.stages[0]);

   const auto& first = pipeline.stages.front();
   if (is_builtin(builtins_, first))
      return run_pipeline_builtin_first(first, pipeline);

   return run_pipeline_all_external(pipeline);
}

} // namespace clanker

