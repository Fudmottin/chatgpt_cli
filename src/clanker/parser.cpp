// src/clanker/parser.cpp
#include "clanker/lexer.h"
#include "clanker/parser.h"

namespace clanker {

static bool is_trailing_control_operator(const LexResult& lr) {
   if (lr.tokens.size() < 2) return false;
   const auto& last = lr.tokens[lr.tokens.size() - 2];
   return last.kind == TokenKind::Pipe || last.kind == TokenKind::AndIf ||
          last.kind == TokenKind::OrIf;
}

ParseResult Parser::parse(const std::string& input) const {
   Lexer lx;
   const LexResult lr = lx.lex(input);

   switch (lr.kind) {
   case LexKind::Incomplete:
      return {.kind = ParseKind::Incomplete};
   case LexKind::Error:
      return {.kind = ParseKind::Error, .message = lr.message};
   case LexKind::Complete:
      break;
   }

   if (is_trailing_control_operator(lr)) return {.kind = ParseKind::Incomplete};

   Pipeline pl;
   pl.stages.push_back(SimpleCommand{});

   for (const Token& t : lr.tokens) {
      switch (t.kind) {
      case TokenKind::Word:
         pl.stages.back().argv.push_back(t.text);
         break;

      case TokenKind::Pipe:
         // start next stage; stage must not be empty
         if (pl.stages.back().argv.empty())
            return {.kind = ParseKind::Error,
                    .message = "empty pipeline stage"};
         pl.stages.push_back(SimpleCommand{});
         break;

      case TokenKind::AndIf:
      case TokenKind::OrIf:
      case TokenKind::Ampersand:
         return {.kind = ParseKind::Error,
                 .message = "operators not implemented yet"};

      case TokenKind::End:
         break;
      }
   }

   if (pl.stages.size() == 1 && pl.stages[0].argv.empty()) {
      // empty input (or comment only)
      return {.kind = ParseKind::Complete, .pipeline = Pipeline{}};
   }

   if (!pl.stages.empty() && pl.stages.back().argv.empty())
      return {.kind = ParseKind::Error, .message = "empty pipeline stage"};

   return {.kind = ParseKind::Complete, .pipeline = std::move(pl)};
}

} // namespace clanker

