#pragma once

#include <filesystem>
#include <string>

namespace clanker {

class Shell {
public:
   Shell();
   int run();

private:
   std::filesystem::path root_;
};

} // namespace clanker

