// src/clanker/shell.h

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace clanker {

class Shell {
 public:
   Shell();

   // REPL
   int run();

   // Batch:
   int run_file(const std::filesystem::path& script_path);
   int run_string(std::string_view script_text);

   const std::filesystem::path& root() const noexcept { return root_; }
   const std::filesystem::path& cwd() const noexcept { return cwd_; }
   const std::filesystem::path& oldpwd() const noexcept { return oldpwd_; }

 private:
   std::filesystem::path root_;
   std::filesystem::path cwd_;
   std::filesystem::path oldpwd_;
};

} // namespace clanker

