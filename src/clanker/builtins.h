#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace clanker {

struct BuiltinContext {
   std::filesystem::path root;
};

using Argv = std::vector<std::string>;
using BuiltinFn = std::function<int(const BuiltinContext&, const Argv&)>;

class Builtins {
public:
   void add(std::string name, BuiltinFn fn, std::string help);
   std::optional<BuiltinFn> find(std::string_view name) const;
   std::vector<std::pair<std::string, std::string>> help_items() const;

private:
   struct Entry {
      BuiltinFn fn;
      std::string help;
   };
   std::unordered_map<std::string, Entry> map_;
};

Builtins make_builtins();

} // namespace clanker

