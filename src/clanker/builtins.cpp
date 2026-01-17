#include "clanker/builtins.h"

#include <algorithm>

namespace clanker {

void Builtins::add(std::string name, BuiltinFn fn, std::string help) {
   map_.insert_or_assign(std::move(name), Entry{std::move(fn), std::move(help)});
}

std::optional<BuiltinFn> Builtins::find(std::string_view name) const {
   auto it = map_.find(std::string{name});
   if (it == map_.end())
      return std::nullopt;
   return it->second.fn;
}

std::vector<std::pair<std::string, std::string>> Builtins::help_items() const {
   std::vector<std::pair<std::string, std::string>> items;
   items.reserve(map_.size());
   for (const auto& [k, v] : map_)
      items.emplace_back(k, v.help);
   std::sort(items.begin(), items.end(),
             [](const auto& a, const auto& b) { return a.first < b.first; });
   return items;
}

void add_core_builtins(Builtins&);
void add_llm_builtins(Builtins&);
void set_help_registry(Builtins&);

Builtins make_builtins() {
   Builtins b;
   add_core_builtins(b);
   add_llm_builtins(b);
   set_help_registry(b);
   return b;
}
} // namespace clanker

