// src/clanker/unique_fd.h

#pragma once

#include <unistd.h>

namespace clanker {

class unique_fd {
 public:
   unique_fd() noexcept = default;
   explicit unique_fd(int fd) noexcept
      : fd_(fd) {}

   unique_fd(const unique_fd&) = delete;
   unique_fd& operator=(const unique_fd&) = delete;

   unique_fd(unique_fd&& other) noexcept
      : fd_(other.release()) {}
   unique_fd& operator=(unique_fd&& other) noexcept {
      if (this != &other) reset(other.release());
      return *this;
   }

   ~unique_fd() { reset(); }

   [[nodiscard]] int get() const noexcept { return fd_; }
   [[nodiscard]] bool valid() const noexcept { return fd_ >= 0; }

   int release() noexcept {
      const int out = fd_;
      fd_ = -1;
      return out;
   }

   void reset(int fd = -1) noexcept {
      if (fd_ >= 0) ::close(fd_);
      fd_ = fd;
   }

 private:
   int fd_{-1};
};

struct pipe_ends {
   unique_fd read;
   unique_fd write;
};

// Returns {read, write} or both invalid on error.
inline pipe_ends make_pipe() noexcept {
   int p[2] = {-1, -1};
   if (::pipe(p) != 0) return {};
   return {unique_fd{p[0]}, unique_fd{p[1]}};
}

} // namespace clanker

