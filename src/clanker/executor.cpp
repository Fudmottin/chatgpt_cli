// src/clanker/executor.cpp

#include <array>
#include <cerrno>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "clanker/executor.h"
#include "clanker/process.h"
#include "clanker/util.h"

namespace clanker {

namespace {

class UniqueFd {
 public:
   UniqueFd() = default;
   explicit UniqueFd(int fd) noexcept
      : fd_(fd) {}

   UniqueFd(const UniqueFd&) = delete;
   UniqueFd& operator=(const UniqueFd&) = delete;

   UniqueFd(UniqueFd&& other) noexcept
      : fd_(other.release()) {}
   UniqueFd& operator=(UniqueFd&& other) noexcept {
      if (this != &other) reset(other.release());
      return *this;
   }

   ~UniqueFd() { reset(-1); }

   [[nodiscard]] int get() const noexcept { return fd_; }
   [[nodiscard]] explicit operator bool() const noexcept { return fd_ >= 0; }

   int release() noexcept {
      const int out = fd_;
      fd_ = -1;
      return out;
   }

   void reset(int new_fd) noexcept {
      if (fd_ >= 0) ::close(fd_);
      fd_ = new_fd;
   }

   void close() noexcept { reset(-1); }

 private:
   int fd_{-1};
};

struct Pipe {
   UniqueFd read;
   UniqueFd write;
};

[[nodiscard]] std::optional<Pipe> make_pipe() noexcept {
   int fds[2] = {-1, -1};
   if (::pipe(fds) != 0) return std::nullopt;
   return Pipe{UniqueFd{fds[0]}, UniqueFd{fds[1]}};
}

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
   auto p = make_pipe();
   if (!p) return 1;

   UniqueFd read_end = std::move(p->read);
   UniqueFd write_end = std::move(p->write);

   // External stages: 1..end (must be external for now)
   std::vector<std::vector<std::string>> external;
   external.reserve(pipeline.stages.size() - 1);

   for (std::size_t i = 1; i < pipeline.stages.size(); ++i) {
      const auto& st = pipeline.stages[i];
      if (st.argv.empty()) return 2;

      if (builtins_.find(st.argv.front())) {
         fd_write_all(STDERR_FILENO,
                      "error: built-ins in non-first pipeline stages "
                      "not implemented yet\n");
         return 2;
      }
      external.push_back(st.argv);
   }

   // Spawn external pipeline (no wait yet), stdin=read_end.
   std::vector<pid_t> pids;
   pids.reserve(external.size());

   UniqueFd prev_read = std::move(read_end);

   for (std::size_t i = 0; i < external.size(); ++i) {
      const bool last = (i + 1 == external.size());

      std::optional<Pipe> next;
      if (!last) {
         next = make_pipe();
         if (!next) return 1;
      }

      const int in_fd = prev_read.get();
      const int out_fd = last ? -1 : next->write.get();

      // Ensure no external child keeps the builtin pipe write end open.
      const std::array<int, 1> close_fds{write_end.get()};
      const int pid_or_err =
         spawn_external(external[i], in_fd, out_fd, close_fds);

      // Parent closes ends it no longer needs.
      prev_read.close();
      if (!last) next->write.close();

      if (pid_or_err < 0) {
         const int err = -pid_or_err;
         if (err == ENOENT) return 127;
         return 126;
      }

      pids.push_back(static_cast<pid_t>(pid_or_err));
      prev_read = last ? UniqueFd{} : std::move(next->read);
   }

   prev_read.close();

   // Run builtin writing to pipe.
   int builtin_status = 0;
   if (auto fn = builtins_.find(first.argv.front())) {
      BuiltinContext ctx{.root = root_,
                         .in_fd = STDIN_FILENO,
                         .out_fd = write_end.get(),
                         .err_fd = STDERR_FILENO,
                         .cwd = cwd_,
                         .oldpwd = oldpwd_};
      builtin_status = (*fn)(ctx, first.argv);
   } else {
      builtin_status = 2;
   }

   // Deliver EOF to external pipeline.
   write_end.close();

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

