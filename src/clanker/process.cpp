#include "clanker/process.h"

#include <cstdlib>

namespace clanker {

int run_external(const std::string& command) {
   const int rc = std::system(command.c_str());
   if (rc == -1)
      return 127;
   return rc;
}

} // namespace clanker

