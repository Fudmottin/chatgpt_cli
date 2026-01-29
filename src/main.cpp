// src/main.cpp

#include <iostream>
#include <string_view>

#include "clanker/shell.h"
#include "clanker/security_policy.h"

namespace {

void usage(std::ostream& os, std::string_view prog) {
   os << "usage:\n"
      << "  " << prog << "            # REPL\n"
      << "  " << prog << " -c CMD     # run CMD, batch mode\n"
      << "  " << prog << " SCRIPT     # run SCRIPT file, batch mode\n";
}

} // namespace

int main(int argc, char** argv) {
   try {
      const auto sec = clanker::SecurityPolicy::capture_startup_identity();
      if (const int ec = sec.refuse_root_start(); ec != 0) {
         std::cerr << "clanker: security: refusing to run as root\n";
         return ec;
      }

      clanker::Shell shell;

      if (argc == 1) {
         return shell.run(); // REPL
      }

      const std::string_view prog = (argc > 0 && argv[0]) ? argv[0] : "clanker";

      if (argc == 3 && std::string_view{argv[1]} == "-c") {
         return shell.run_string(argv[2]);
      }

      if (argc == 2) {
         return shell.run_file(argv[1]);
      }

      usage(std::cerr, prog);
      return 2;
   } catch (const std::exception& e) {
      std::cerr << "fatal: " << e.what() << '\n';
      return 1;
   }
}

