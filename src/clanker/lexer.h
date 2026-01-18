// src/clanker/lexer.h

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace clanker {

struct SourceLoc {
   std::size_t index{0};
   std::size_t line{1};
   std::size_t column{1};
};

enum class TokenKind {
   Word,
   Pipe,      // |
   AndIf,     // &&
   OrIf,      // ||
   Ampersand, // &
   End        // sentinel
};

struct Token {
   TokenKind kind{TokenKind::End};
   std::string text; // for Word; empty for operators
   SourceLoc loc{};
};

enum class LexKind { Complete, Incomplete, Error };

struct LexResult {
   LexKind kind{LexKind::Error};
   std::vector<Token> tokens;
   std::string message;
   SourceLoc error_loc{};
};

class Lexer {
 public:
   LexResult lex(std::string_view input) const;

 private:
   static bool is_space(char c) noexcept;
};

} // namespace clanker

