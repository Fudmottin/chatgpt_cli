// src/clanker/shell.h

#pragma once

#include <filesystem>
#include <string>

namespace clanker {

class Shell {
 public:
   Shell();
   int run();
   const std::filesystem::path& root() const noexcept { return root_; }
   const std::filesystem::path& cwd() const noexcept { return cwd_; }
   const std::filesystem::path& oldpwd() const noexcept { return oldpwd_; }

 private:
   std::filesystem::path root_;
   std::filesystem::path cwd_;
   std::filesystem::path oldpwd_;
};

} // namespace clanker

