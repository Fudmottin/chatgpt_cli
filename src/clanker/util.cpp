#include "clanker/util.h"

#include <cctype>
#include <charconv>

namespace clanker {

std::string rtrim(const std::string& s) {
   std::size_t end = s.size();
   while (end > 0 && std::isspace(static_cast<unsigned char>(s[end - 1])))
      --end;
   return s.substr(0, end);
}

std::string trim(const std::string& s) {
   std::size_t b = 0;
   while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
      ++b;
   std::size_t e = s.size();
   while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
      --e;
   return s.substr(b, e - b);
}

bool ends_with(const std::string& s, const std::string& suffix) {
   if (suffix.size() > s.size())
      return false;
   return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::vector<std::string> split_ws(const std::string& s) {
   std::vector<std::string> out;
   std::size_t i = 0;
   while (i < s.size()) {
      while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
         ++i;
      if (i >= s.size())
         break;
      std::size_t j = i;
      while (j < s.size() && !std::isspace(static_cast<unsigned char>(s[j])))
         ++j;
      out.emplace_back(s.substr(i, j - i));
      i = j;
   }
   return out;
}

std::vector<std::string> split_lines(const std::string& s) {
   std::vector<std::string> out;
   std::string cur;
   for (char c : s) {
      if (c == '\n') {
         out.push_back(cur);
         cur.clear();
      } else {
         cur.push_back(c);
      }
   }
   out.push_back(cur);
   return out;
}

std::optional<int> to_int(const std::string& s) {
   int v = 0;
   const auto* b = s.data();
   const auto* e = s.data() + s.size();
   auto [ptr, ec] = std::from_chars(b, e, v);
   if (ec != std::errc{} || ptr != e)
      return std::nullopt;
   return v;
}

} // namespace clanker

