// src/tests/test_main.cpp

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
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

   if (WIFEXITED(status))
      rr.exit_code = WEXITSTATUS(status);
   else
      rr.exit_code = 128;

   return rr;
}

[[noreturn]] void usage() {
   std::cerr << "usage: clanker_tests /path/to/clanker [--case NAME]\n"
             << "cases:\n"
             << "  smoke\n"
             << "  pipeline\n"
             << "  list\n"
             << "  status\n"
             << "  andor\n"
             << "  background\n";
   std::exit(2);
}

void expect(bool ok, std::string_view msg) {
   if (!ok) {
      std::cerr << "FAIL: " << msg << '\n';
      std::exit(1);
   }
}

std::string_view get_case(int argc, char** argv) {
   // Default: run everything.
   for (int i = 2; i < argc; ++i) {
      const std::string_view a = argv[i];
      if (a == "--case") {
         if (i + 1 >= argc) usage();
         return std::string_view(argv[i + 1]);
      }
   }
   return "all";
}

// ---- Test groups ----

void test_smoke(const char* clanker) {
   const auto rr = run_clanker(clanker, "echo hi");
   expect(rr.exit_code == 0, "echo exit code");
   expect(rr.out == "hi\n", "echo stdout");
   expect(rr.err.empty(), "echo stderr empty");
}

void test_pipeline(const char* clanker) {
   const auto rr = run_clanker(clanker, "echo a|cat");
   expect(rr.exit_code == 0, "pipeline exit code");
   expect(rr.out == "a\n", "pipeline stdout");
   expect(rr.err.empty(), "pipeline stderr empty");
}

void test_list(const char* clanker) {
   {
      const auto rr = run_clanker(clanker, "echo a; echo b");
      expect(rr.exit_code == 0, "list ';' exit code");
      expect(rr.out == "a\nb\n", "list ';' stdout");
      expect(rr.err.empty(), "list ';' stderr empty");
   }
   {
      const auto rr = run_clanker(clanker, "echo a;echo b");
      expect(rr.exit_code == 0, "list adjacency exit code");
      expect(rr.out == "a\nb\n", "list adjacency stdout");
      expect(rr.err.empty(), "list adjacency stderr empty");
   }
   {
      const auto rr = run_clanker(clanker, "echo a; echo b;");
      expect(rr.exit_code == 0, "list trailing ';' exit code");
      expect(rr.out == "a\nb\n", "list trailing ';' stdout");
      expect(rr.err.empty(), "list trailing ';' stderr empty");
   }
   {
      const auto rr = run_clanker(clanker, "echo a\necho b");
      expect(rr.exit_code == 0, "list newline exit code");
      expect(rr.out == "a\nb\n", "list newline stdout");
      expect(rr.err.empty(), "list newline stderr empty");
   }
}

void test_status(const char* clanker) {
   {
      const auto rr = run_clanker(clanker, "false; true");
      expect(rr.exit_code == 0, "status last wins (false; true)");
   }
   {
      const auto rr = run_clanker(clanker, "true; false");
      expect(rr.exit_code != 0, "status last wins (true; false)");
   }
}

void test_andor(const char* clanker) {
   {
      const auto rr = run_clanker(clanker, "false&&echo x");
      expect(rr.exit_code != 0, "false&&... exit code");
      expect(rr.out.empty(), "false&&... stdout empty");
   }
   {
      const auto rr = run_clanker(clanker, "true&&echo x");
      expect(rr.exit_code == 0, "true&&... exit code");
      expect(rr.out == "x\n", "true&&... stdout");
   }
   {
      const auto rr = run_clanker(clanker, "false||echo x");
      expect(rr.exit_code == 0, "false||... exit code");
      expect(rr.out == "x\n", "false||... stdout");
   }
   {
      const auto rr = run_clanker(clanker, "true||echo x");
      expect(rr.exit_code == 0, "true||... exit code");
      expect(rr.out.empty(), "true||... stdout empty");
   }

   // Chaining / precedence: left-to-right AndOr chain (false && X) || Y => Y.
   {
      const auto rr = run_clanker(clanker, "false && echo x || echo y");
      expect(rr.exit_code == 0, "false&&...||... exit code");
      expect(rr.out == "y\n", "false&&...||... stdout");
   }

   // Pipeline status feeds control operators: (echo a | cat) && echo b.
   {
      const auto rr = run_clanker(clanker, "echo a | cat && echo b");
      expect(rr.exit_code == 0, "pipeline && exit code");
      expect(rr.out == "a\nb\n", "pipeline && stdout");
      expect(rr.err.empty(), "pipeline && stderr empty");
   }
}

static bool contains_line(const std::string& out, std::string_view line) {
   // Cheap and cheerful: exact substring check.
   return out.find(std::string(line)) != std::string::npos;
}

void test_background(const char* clanker) {
   // Order is intentionally not asserted.
   const auto rr = run_clanker(clanker, "echo a & echo b");
   expect(rr.exit_code == 0, "background exit code");
   expect(contains_line(rr.out, "a\n"), "background contains a");
   expect(contains_line(rr.out, "b\n"), "background contains b");
}

} // namespace

int main(int argc, char** argv) {
   if (argc < 2) usage();
   const char* clanker = argv[1];

   const std::string_view which = get_case(argc, argv);

   if (which == "all") {
      test_smoke(clanker);
      test_pipeline(clanker);
      test_list(clanker);
      test_status(clanker);
      test_andor(clanker);
      test_background(clanker);
   } else if (which == "smoke") {
      test_smoke(clanker);
   } else if (which == "pipeline") {
      test_pipeline(clanker);
   } else if (which == "list") {
      test_list(clanker);
   } else if (which == "status") {
      test_status(clanker);
   } else if (which == "andor") {
      test_andor(clanker);
   } else if (which == "background") {
      test_background(clanker);
   } else {
      usage();
   }

   std::cout << "OK\n";
   return 0;
}

