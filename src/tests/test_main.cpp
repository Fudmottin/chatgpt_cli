#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct RunResult {
   int exit_code{};
   std::string out;
   std::string err;
};

std::string read_all(int fd) {
   std::string s;
   char buf[4096];
   for (;;) {
      const auto n = ::read(fd, buf, sizeof(buf));
      if (n == 0) break;
      if (n < 0) {
         if (errno == EINTR) continue;
         break;
      }
      s.append(buf, buf + n);
   }
   return s;
}

RunResult run_clanker(const char* clanker_path, std::string_view cmd) {
   int out_pipe[2]{}, err_pipe[2]{};
   if (::pipe(out_pipe) != 0) throw std::runtime_error("pipe(stdout) failed");
   if (::pipe(err_pipe) != 0) throw std::runtime_error("pipe(stderr) failed");

   const pid_t pid = ::fork();
   if (pid < 0) throw std::runtime_error("fork failed");

   if (pid == 0) {
      ::dup2(out_pipe[1], STDOUT_FILENO);
      ::dup2(err_pipe[1], STDERR_FILENO);

      ::close(out_pipe[0]);
      ::close(out_pipe[1]);
      ::close(err_pipe[0]);
      ::close(err_pipe[1]);

      const std::string cmd_str(cmd);
      char* const argv[] = {
         const_cast<char*>(clanker_path),
         const_cast<char*>("-c"),
         const_cast<char*>(cmd_str.c_str()),
         nullptr,
      };

      ::execv(clanker_path, argv);
      _exit(127);
   }

   ::close(out_pipe[1]);
   ::close(err_pipe[1]);

   RunResult rr;
   rr.out = read_all(out_pipe[0]);
   rr.err = read_all(err_pipe[0]);

   ::close(out_pipe[0]);
   ::close(err_pipe[0]);

   int status = 0;
   while (::waitpid(pid, &status, 0) < 0) {
      if (errno == EINTR) continue;
      throw std::runtime_error("waitpid failed");
   }

   if (WIFEXITED(status)) rr.exit_code = WEXITSTATUS(status);
   else rr.exit_code = 128;

   return rr;
}

void expect(bool ok, std::string_view msg) {
   if (!ok) {
      std::cerr << "FAIL: " << msg << '\n';
      std::exit(1);
   }
}

} // namespace

int main(int argc, char** argv) {
   expect(argc == 2, "usage: clanker_tests /path/to/clanker");
   const char* clanker = argv[1];

   {
      const auto rr = run_clanker(clanker, "echo hi");
      expect(rr.exit_code == 0, "echo exit code");
      expect(rr.out == "hi\n", "echo stdout");
   }

   {
      const auto rr = run_clanker(clanker, "echo a|cat");
      expect(rr.exit_code == 0, "pipeline exit code");
      expect(rr.out == "a\n", "pipeline stdout");
   }

   {
      const auto rr = run_clanker(clanker, "false&&echo x");
      expect(rr.exit_code != 0, "false&&... exit code");
      expect(rr.out.empty(), "false&&... stdout empty");
   }

   std::cout << "OK\n";
   return 0;
}

