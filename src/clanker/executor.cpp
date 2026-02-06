// src/clanker/executor.cpp
#include <cerrno>
#include <cstring>
#include <fcntl.h>
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

static int deny_privilege_drift() {
   fd_write_all(
      STDERR_FILENO,
      "clanker: security: privilege change detected; refusing to execute\n");
   return 125;
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

int open_redir_fd(const Redirection& r) {
   int flags = 0;
   switch (r.kind) {
   case RedirKind::In:
      flags = O_RDONLY;
      break;
   case RedirKind::OutTrunc:
      flags = O_WRONLY | O_CREAT | O_TRUNC;
      break;
   case RedirKind::OutAppend:
      flags = O_WRONLY | O_CREAT | O_APPEND;
      break;
   }
   flags |= O_CLOEXEC;

   const int fd = ::open(r.target.c_str(), flags, 0666);
   if (fd < 0) return -errno;
   return fd;
}

bool is_std_fd(int fd) noexcept { return fd == 0 || fd == 1 || fd == 2; }

int apply_redirs_to_spawn(const std::vector<Redirection>& redirs, int& in_fd,
                          int& out_fd, int& err_fd, UniqueFd& in_owner,
                          UniqueFd& out_owner, UniqueFd& err_owner,
                          std::string& err_msg) {
   for (const auto& r : redirs) {
      if (!is_std_fd(r.fd)) {
         err_msg = "error: redirection fd not supported (only 0,1,2)\n";
         return 2;
      }

      const int ofd = open_redir_fd(r);
      if (ofd < 0) {
         err_msg = "error: cannot open '" + r.target + "': " +
                   std::string(::strerror(-ofd)) + "\n";
         return 1;
      }

      UniqueFd opened(ofd);

      if (r.fd == 0) {
         in_owner = std::move(opened);
         in_fd = in_owner.get();
      } else if (r.fd == 1) {
         out_owner = std::move(opened);
         out_fd = out_owner.get();
      } else { // 2
         err_owner = std::move(opened);
         err_fd = err_owner.get();
      }
   }

   return 0;
}

int apply_redirs_in_process(const std::vector<Redirection>& redirs,
                            UniqueFd& save0, UniqueFd& save1, UniqueFd& save2,
                            std::string& err_msg) {
   auto save_if_needed = [&](int fd, UniqueFd& save) -> bool {
      if (save.get() != -1) return true;
      const int d = ::dup(fd);
      if (d < 0) return false;
      save.reset(d);
      return true;
   };

   for (const auto& r : redirs) {
      if (!is_std_fd(r.fd)) {
         err_msg = "error: redirection fd not supported (only 0,1,2)\n";
         return 2;
      }

      if (r.fd == 0 && !save_if_needed(0, save0)) return 1;
      if (r.fd == 1 && !save_if_needed(1, save1)) return 1;
      if (r.fd == 2 && !save_if_needed(2, save2)) return 1;

      const int ofd = open_redir_fd(r);
      if (ofd < 0) {
         err_msg = "error: cannot open '" + r.target + "': " +
                   std::string(::strerror(-ofd)) + "\n";
         return 1;
      }

      UniqueFd opened(ofd);

      if (::dup2(opened.get(), r.fd) < 0) {
         err_msg = "error: dup2 failed\n";
         return 1;
      }
   }

   return 0;
}

void restore_std_fds(UniqueFd& save0, UniqueFd& save1, UniqueFd& save2) {
   if (save0.get() != -1) {
      (void)::dup2(save0.get(), 0);
      save0.reset();
   }
   if (save1.get() != -1) {
      (void)::dup2(save1.get(), 1);
      save1.reset();
   }
   if (save2.get() != -1) {
      (void)::dup2(save2.get(), 2);
      save2.reset();
   }
}

} // namespace

Executor::Executor(Builtins builtins, const ExecPolicy& policy,
                   std::filesystem::path* cwd, std::filesystem::path* oldpwd,
                   SecurityPolicy sec)
   : builtins_(std::move(builtins))
   , policy_(policy)
   , sec_(sec)
   , cwd_(cwd)
   , oldpwd_(oldpwd) {}

int Executor::run_simple(const SimpleCommand& cmd) {
   // Allow redirection-only commands.
   if (cmd.argv.empty()) {
      if (cmd.redirs.empty()) return 0;
      UniqueFd save0, save1, save2;
      std::string em;
      const int rc =
         apply_redirs_in_process(cmd.redirs, save0, save1, save2, em);
      if (rc != 0) {
         if (em.empty()) em = "error: redirection failed\n";
         fd_write_all(STDERR_FILENO, em);
         restore_std_fds(save0, save1, save2);
         return (rc == 2) ? 2 : 1;
      }
      // For now, do not persist redirections in the shell process.
      restore_std_fds(save0, save1, save2);
      return 0;
   }

   if (!sec_.identity_unchanged()) return deny_privilege_drift();

   // Built-in
   if (auto fn = builtins_.find(cmd.argv.front())) {
      UniqueFd save0, save1, save2;
      std::string em;
      const int rc =
         apply_redirs_in_process(cmd.redirs, save0, save1, save2, em);
      if (rc != 0) {
         if (em.empty()) em = "error: redirection failed\n";
         fd_write_all(STDERR_FILENO, em);
         restore_std_fds(save0, save1, save2);
         return (rc == 2) ? 2 : 1;
      }

      BuiltinContext ctx{.root = policy_.root(),
                         .in_fd = STDIN_FILENO,
                         .out_fd = STDOUT_FILENO,
                         .err_fd = STDERR_FILENO,
                         .cwd = cwd_,
                         .oldpwd = oldpwd_};

      const int st = (*fn)(ctx, cmd.argv);

      restore_std_fds(save0, save1, save2);
      return st;
   }

   // External
   std::string reason;
   if (!policy_.allow_external(cmd.argv, reason)) {
      if (reason.empty()) reason = "disallowed by policy";
      fd_write_all(STDERR_FILENO, "error: " + reason + "\n");
      return 126;
   }

   UniqueFd in_owner, out_owner, err_owner;
   int in_fd = -1, out_fd = -1, err_fd = -1;
   std::string em;
   const int rc = apply_redirs_to_spawn(cmd.redirs, in_fd, out_fd, err_fd,
                                        in_owner, out_owner, err_owner, em);
   if (rc != 0) {
      if (em.empty()) em = "error: redirection failed\n";
      fd_write_all(STDERR_FILENO, em);
      return (rc == 2) ? 2 : 1;
   }

   SpawnSpec spec;
   spec.argv = cmd.argv;
   spec.stdin_fd = in_fd;
   spec.stdout_fd = out_fd;
   spec.stderr_fd = err_fd;

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
   if (!sec_.identity_unchanged()) return deny_privilege_drift();

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
      if (st.argv.empty() && st.redirs.empty()) return 2;

      if (!st.argv.empty() && builtins_.find(st.argv.front())) {
         fd_write_all(STDERR_FILENO,
                      "error: built-ins in non-first pipeline stages "
                      "not implemented yet\n");
         return 2;
      }

      if (!st.argv.empty()) {
         std::string reason;
         if (!policy_.allow_external(st.argv, reason)) {
            if (reason.empty()) reason = "disallowed by policy";
            fd_write_all(STDERR_FILENO, "error: " + reason + "\n");
            return 126;
         }
      }

      UniqueFd next_read;
      UniqueFd next_write;
      const bool last = (i + 1 == pipeline.stages.size());

      if (!last) {
         if (int ec = make_pipe(next_read, next_write); ec != 0) return 1;
      }

      // Redirection-only stage: treat as no-op.
      if (st.argv.empty()) {
         prev_read.reset();
         if (!last) {
            next_write.reset();
            prev_read = std::move(next_read);
         }
         continue;
      }

      SpawnSpec spec;
      spec.argv = st.argv;
      spec.stdin_fd = prev_read.get();
      spec.stdout_fd = last ? -1 : next_write.get();
      spec.stderr_fd = -1;

      // Ensure the external child does not keep the builtin write end open.
      spec.close_fds.push_back(write_end.get());

      // Apply stage redirections (override pipe defaults).
      {
         UniqueFd rin, rout, rerr;
         int in_fd = spec.stdin_fd;
         int out_fd = spec.stdout_fd;
         int err_fd = spec.stderr_fd;
         std::string em;
         const int rc =
            apply_redirs_to_spawn(st.redirs, in_fd, out_fd, err_fd, rin, rout,
                                  rerr, em);
         if (rc != 0) {
            if (em.empty()) em = "error: redirection failed\n";
            fd_write_all(STDERR_FILENO, em);
            return (rc == 2) ? 2 : 1;
         }
         spec.stdin_fd = in_fd;
         spec.stdout_fd = out_fd;
         spec.stderr_fd = err_fd;
      }

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

   // Run builtin with stdout wired to the pipe, unless redirections override it.
   int builtin_status = 0;
   if (auto fn = builtins_.find(first.argv.front())) {
      UniqueFd save0, save1, save2;

      // Always save the std fds we might touch.
      save0.reset(::dup(STDIN_FILENO));
      save1.reset(::dup(STDOUT_FILENO));
      save2.reset(::dup(STDERR_FILENO));
      if (save0.get() < 0 || save1.get() < 0 || save2.get() < 0) {
         fd_write_all(STDERR_FILENO, "error: dup failed\n");
         restore_std_fds(save0, save1, save2);
         return 1;
      }

      // Pipe is the default stdout for the builtin stage.
      if (::dup2(write_end.get(), STDOUT_FILENO) < 0) {
         fd_write_all(STDERR_FILENO, "error: dup2 failed\n");
         restore_std_fds(save0, save1, save2);
         return 1;
      }

      // Apply explicit redirections (override the pipe if they touch fd 1).
      std::string em;
      const int rc =
         apply_redirs_in_process(first.redirs, save0, save1, save2, em);
      if (rc != 0) {
         if (em.empty()) em = "error: redirection failed\n";
         fd_write_all(STDERR_FILENO, em);
         restore_std_fds(save0, save1, save2);
         return (rc == 2) ? 2 : 1;
      }

      BuiltinContext ctx{.root = policy_.root(),
                         .in_fd = STDIN_FILENO,
                         .out_fd = STDOUT_FILENO,
                         .err_fd = STDERR_FILENO,
                         .cwd = cwd_,
                         .oldpwd = oldpwd_};

      builtin_status = (*fn)(ctx, first.argv);

      restore_std_fds(save0, save1, save2);
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
   if (!sec_.identity_unchanged()) return deny_privilege_drift();

   // Validate and policy-check first (fail fast, no partial execution).
   for (const auto& st : pipeline.stages) {
      if (st.argv.empty() && st.redirs.empty()) return 2;

      if (!st.argv.empty() && builtins_.find(st.argv.front())) {
         fd_write_all(STDERR_FILENO,
                      "error: built-ins in pipelines not implemented yet\n");
         return 2;
      }

      if (!st.argv.empty()) {
         std::string reason;
         if (!policy_.allow_external(st.argv, reason)) {
            if (reason.empty()) reason = "disallowed by policy";
            fd_write_all(STDERR_FILENO, "error: " + reason + "\n");
            return 126;
         }
      }
   }

   std::vector<pid_t> pids;
   pids.reserve(pipeline.stages.size());

   UniqueFd prev_read;

   for (std::size_t i = 0; i < pipeline.stages.size(); ++i) {
      const auto& st = pipeline.stages[i];
      const bool last = (i + 1 == pipeline.stages.size());

      UniqueFd next_read;
      UniqueFd next_write;
      if (!last) {
         if (int ec = make_pipe(next_read, next_write); ec != 0) return 1;
      }

      SpawnSpec spec;

      // Redirection-only stage in a pipeline: treat as no-op.
      // (You can refine semantics later; this keeps execution deterministic.)
      if (st.argv.empty()) {
         // Close pipe fds as if this stage consumed/produced nothing.
         // Upstream writer will see EOF once parent closes its copies.
         prev_read.reset();
         if (!last) {
            next_write.reset();
            prev_read = std::move(next_read);
         }
         continue;
      }

      spec.argv = st.argv;
      spec.stdin_fd = prev_read.get();
      spec.stdout_fd = last ? -1 : next_write.get();
      spec.stderr_fd = -1;

      // Apply stage redirections (override pipe defaults for 0/1/2).
      {
         UniqueFd rin, rout, rerr;
         int in_fd = spec.stdin_fd;
         int out_fd = spec.stdout_fd;
         int err_fd = spec.stderr_fd;
         std::string em;
         const int rc = apply_redirs_to_spawn(st.redirs, in_fd, out_fd, err_fd,
                                              rin, rout, rerr, em);
         if (rc != 0) {
            if (em.empty()) em = "error: redirection failed\n";
            fd_write_all(STDERR_FILENO, em);
            return (rc == 2) ? 2 : 1;
         }
         spec.stdin_fd = in_fd;
         spec.stdout_fd = out_fd;
         spec.stderr_fd = err_fd;
      }

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

int Executor::run_andor(const AndOr& ao) {
   int st = run_pipeline(ao.first);

   for (const auto& tail : ao.rest) {
      const bool ok = (st == 0);
      if (tail.op == AndOrOp::AndIf) {
         if (!ok) continue;
      } else { // OrIf
         if (ok) continue;
      }
      st = run_pipeline(tail.rhs);
   }

   return st;
}

int Executor::run_background(const AndOr& ao) {
   if (!sec_.identity_unchanged()) return deny_privilege_drift();

   const pid_t pid = ::fork();
   if (pid < 0) {
      fd_write_all(STDERR_FILENO, "clanker: fork failed\n");
      return 1; // deterministic: failed to start
   }

   if (pid == 0) {
      const int st = run_andor(ao);
      _exit(st & 0xff); // deterministic, avoid flushing parent buffers
   }

   // Parent: do not wait. Deterministic status: started successfully.
   return 0;
}

int Executor::run_list(const CommandList& list) {
   int last_status = 0;

   for (const auto& it : list.items) {
      if (it.term == Terminator::Ampersand) {
         last_status = run_background(it.cmd); // 0 if started, else 1/125/etc.
      } else {
         last_status = run_andor(it.cmd);
      }
   }

   return last_status;
}

} // namespace clanker

