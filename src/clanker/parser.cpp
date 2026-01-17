#include "clanker/parser.h"

#include "clanker/util.h"

namespace clanker {

static bool is_escaped(const std::string& s, std::size_t i) {
   // true if s[i] is preceded by an odd number of backslashes
   std::size_t n = 0;
   while (i > 0 && s[i - 1] == '\\') {
      ++n;
      --i;
   }
   return (n % 2) == 1;
}

bool Parser::has_unclosed_quotes(const std::string& s) {
   bool in_single = false;
   bool in_double = false;

   for (std::size_t i = 0; i < s.size(); ++i) {
      const char c = s[i];
      if (c == '\'' && !in_double && !is_escaped(s, i))
         in_single = !in_single;
      else if (c == '"' && !in_single && !is_escaped(s, i))
         in_double = !in_double;
   }
   return in_single || in_double;
}

bool Parser::ends_with_escaped_newline(const std::string& input) {
   // Incomplete if the last *non-empty* line ends with an unescaped backslash.
   auto lines = split_lines(input);
   for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
      const auto t = rtrim(*it);
      if (t.empty())
         continue;
      if (!t.empty() && t.back() == '\\') {
         const std::size_t i = t.size() - 1;
         return !is_escaped(t, i);
      }
      return false;
   }
   return false;
}

bool Parser::ends_with_dangling_operator(const std::string& s0) {
   const auto s = rtrim(s0);
   if (s.empty())
      return false;

   if (ends_with(s, "&&") || ends_with(s, "||"))
      return true;

   // dangling pipe if last non-space is '|'
   return s.back() == '|';
}

ParseResult Parser::parse(const std::string& input) const {
   // This is intentionally a minimal “completeness” parser.
   // A real lexer/parser will replace it; keep the interface stable.

   if (has_unclosed_quotes(input) || ends_with_escaped_newline(input) ||
       ends_with_dangling_operator(input)) {
      return {.kind = ParseKind::Incomplete};
   }

   // For now, the “logical command” is just input with newlines preserved.
   // Executor will treat it as a single command line (temporary).
   return {.kind = ParseKind::Complete, .logical_command = input};
}

} // namespace clanker

