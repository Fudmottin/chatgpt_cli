#include "clanker/shell.h"

#include <iostream>

int main() {
   try {
      clanker::Shell shell;
      return shell.run();
   } catch (const std::exception& e) {
      std::cerr << "fatal: " << e.what() << '\n';
      return 1;
   }
}

