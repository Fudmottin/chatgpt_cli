#pragma once

#include <string>

namespace clanker {

enum class ParseKind { Complete, Incomplete, Error };

struct ParseResult {
   ParseKind kind{ParseKind::Error};
   std::string logical_command; // valid when Complete
   std::string message;         // valid when Error
};

class Parser {
public:
   ParseResult parse(const std::string& input) const;

private:
   static bool ends_with_dangling_operator(const std::string& s);
   static bool has_unclosed_quotes(const std::string& s);
   static bool ends_with_escaped_newline(const std::string& s);
};

} // namespace clanker

