// src/clanker/parser.cpp
#include <utility>

#include "clanker/lexer.h"
#include "clanker/parser.h"

namespace clanker {

static bool is_trailing_control_operator(const LexResult& lr) {
   if (lr.tokens.size() < 2) return false;
   const auto& last = lr.tokens[lr.tokens.size() - 2];
   return last.kind == TokenKind::Pipe || last.kind == TokenKind::AndIf ||
          last.kind == TokenKind::OrIf;
}

static bool pipeline_is_empty(const Pipeline& pl) {
   if (pl.stages.empty()) return true;
   if (pl.stages.size() == 1 && pl.stages[0].argv.empty()) return true;
   for (const auto& st : pl.stages) {
      if (!st.argv.empty()) return false;
   }
   return true;
}

static const char* token_spelling(TokenKind k) noexcept {
   switch (k) {
   case TokenKind::Word:
      return "word";
   case TokenKind::Pipe:
      return "|";
   case TokenKind::AndIf:
      return "&&";
   case TokenKind::OrIf:
      return "||";
   case TokenKind::Ampersand:
      return "&";
   case TokenKind::Semicolon:
      return ";";
   case TokenKind::Newline:
      return "newline";
   case TokenKind::End:
      return "<end>";
   }
   return "<unknown>";
}

static ParseResult parse_error(std::string msg) {
   return {.kind = ParseKind::Error, .message = std::move(msg)};
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

   // Trailing control operators require more input.
   // Trailing ';' or NEWLINE is complete (it terminates a list).
   if (is_trailing_control_operator(lr)) return {.kind = ParseKind::Incomplete};

   CommandList list;
   bool saw_terminator = false;

   Pipeline current;
   current.stages.push_back(SimpleCommand{});

   auto reset_current = [&] {
      current = Pipeline{};
      current.stages.push_back(SimpleCommand{});
   };

   auto push_current_if_nonempty = [&]() -> ParseResult {
      if (pipeline_is_empty(current)) {
         reset_current();
         return {.kind = ParseKind::Complete};
      }

      if (!current.stages.empty() && current.stages.back().argv.empty()) {
         reset_current();
         return parse_error("syntax error: empty pipeline stage");
      }

      list.pipelines.push_back(std::move(current));
      reset_current();
      return {.kind = ParseKind::Complete};
   };

   for (const Token& t : lr.tokens) {
      switch (t.kind) {
      case TokenKind::Word:
         current.stages.back().argv.push_back(t.text);
         break;

      case TokenKind::Pipe:
         if (current.stages.back().argv.empty()) {
            return parse_error("syntax error: empty pipeline stage before '|'");
         }
         current.stages.push_back(SimpleCommand{});
         break;

      case TokenKind::Semicolon:
      case TokenKind::Newline: {
         saw_terminator = true;
         const ParseResult r = push_current_if_nonempty();
         if (r.kind == ParseKind::Error) return r;
         break;
      }

      case TokenKind::AndIf:
      case TokenKind::OrIf:
      case TokenKind::Ampersand:
         return parse_error(std::string("syntax error: operator '") +
                            token_spelling(t.kind) +
                            "' is recognized but not implemented");

      case TokenKind::End:
         break;
      }
   }

   // Finalize trailing pipeline at EOF.
   if (!pipeline_is_empty(current)) {
      if (!current.stages.empty() && current.stages.back().argv.empty()) {
         return parse_error(
            "syntax error: empty pipeline stage at end of input");
      }
      list.pipelines.push_back(std::move(current));
   }

   // Empty input (or comment-only): keep legacy behavior.
   if (list.pipelines.empty()) {
      return {.kind = ParseKind::Complete, .pipeline = Pipeline{}};
   }

   // Backward compatibility: no terminator and exactly one pipeline => legacy.
   if (!saw_terminator && list.pipelines.size() == 1) {
      Pipeline pl = std::move(list.pipelines.front());
      return {.kind = ParseKind::Complete, .pipeline = std::move(pl)};
   }

   return {.kind = ParseKind::Complete, .list = std::move(list)};
}

} // namespace clanker

