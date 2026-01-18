// src/clanker/lexer.cpp

#include <string>

#include "clanker/lexer.h"

namespace clanker {

namespace {

struct Cursor {
   std::string_view s;
   std::size_t i{0};
   SourceLoc loc{0, 1, 1};

   bool eof() const noexcept { return i >= s.size(); }
   char peek() const noexcept { return eof() ? '\0' : s[i]; }

   void advance() noexcept {
      if (eof()) return;
      const char c = s[i++];
      ++loc.index;
      if (c == '\n') {
         ++loc.line;
         loc.column = 1;
      } else {
         ++loc.column;
      }
   }

   bool consume(char c) noexcept {
      if (peek() != c) return false;
      advance();
      return true;
   }
};

} // namespace

bool Lexer::is_space(char c) noexcept {
   return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static LexResult error_at(const std::string& msg, const SourceLoc& loc) {
   LexResult r;
   r.kind = LexKind::Error;
   r.message = msg;
   r.error_loc = loc;
   return r;
}

static LexResult incomplete_at(const SourceLoc& loc) {
   LexResult r;
   r.kind = LexKind::Incomplete;
   r.error_loc = loc;
   return r;
}

LexResult Lexer::lex(std::string_view input) const {
   Cursor cur{input};

   LexResult out;
   out.kind = LexKind::Complete;

   auto push_op = [&](TokenKind k, SourceLoc loc) {
      out.tokens.push_back(Token{.kind = k, .text = {}, .loc = loc});
   };

   auto skip_ws = [&]() {
      while (!cur.eof() && is_space(cur.peek())) cur.advance();
   };

   auto skip_comment = [&]() {
      // assumes current char is '#'
      while (!cur.eof() && cur.peek() != '\n') cur.advance();
   };

   auto lex_word = [&]() -> LexResult {
      const SourceLoc start = cur.loc;
      std::string w;

      bool in_single = false;
      bool in_double = false;

      auto append = [&](char c) { w.push_back(c); };

      while (!cur.eof()) {
         const char c = cur.peek();

         if (!in_single && !in_double) {
            if (is_space(c)) break;
            if (c ==
                '#') // comment begins at token boundary or after whitespace
               break;
            if (c == '|') break;
            if (c == '&') break;
            if (c == '\'') {
               in_single = true;
               cur.advance();
               continue;
            }
            if (c == '"') {
               in_double = true;
               cur.advance();
               continue;
            }
            if (c == '\\') {
               const SourceLoc esc_loc = cur.loc;
               cur.advance();
               if (cur.eof()) return incomplete_at(esc_loc);
               const char n = cur.peek();
               if (n == '\n') {
                  // escaped newline: remove it (treat as nothing)
                  cur.advance();
                  continue;
               }
               // generic escape: take next char literally
               append(n);
               cur.advance();
               continue;
            }

            append(c);
            cur.advance();
            continue;
         }

         if (in_single) {
            if (c == '\'') {
               in_single = false;
               cur.advance();
               continue;
            }
            // literal
            append(c);
            cur.advance();
            continue;
         }

         // in_double
         if (c == '"') {
            in_double = false;
            cur.advance();
            continue;
         }
         if (c == '\\') {
            const SourceLoc esc_loc = cur.loc;
            cur.advance();
            if (cur.eof()) return incomplete_at(esc_loc);
            const char n = cur.peek();
            if (n == '\n') {
               cur.advance();
               continue;
            }
            switch (n) {
            case '"':
               append('"');
               break;
            case '\\':
               append('\\');
               break;
            case 'n':
               append('\n');
               break;
            default:
               // Spec says other escapes are unspecified; reject for now.
               return error_at("unsupported escape in double quotes", esc_loc);
            }
            cur.advance();
            continue;
         }

         append(c);
         cur.advance();
      }

      if (in_single || in_double) return incomplete_at(start);

      if (w.empty()) return error_at("expected word", start);

      out.tokens.push_back(
         Token{.kind = TokenKind::Word, .text = std::move(w), .loc = start});
      return LexResult{.kind = LexKind::Complete};
   };

   while (!cur.eof()) {
      skip_ws();
      if (cur.eof()) break;

      const char c = cur.peek();

      if (c == '#') {
         skip_comment();
         continue;
      }

      if (c == '|') {
         const SourceLoc loc = cur.loc;
         cur.advance();
         if (cur.consume('|')) {
            push_op(TokenKind::OrIf, loc);
         } else {
            push_op(TokenKind::Pipe, loc);
         }
         continue;
      }

      if (c == '&') {
         const SourceLoc loc = cur.loc;
         cur.advance();
         if (cur.consume('&')) {
            push_op(TokenKind::AndIf, loc);
         } else {
            push_op(TokenKind::Ampersand, loc);
         }
         continue;
      }

      // word
      {
         LexResult r = lex_word();
         if (r.kind == LexKind::Error) return r;
         if (r.kind == LexKind::Incomplete) return r;
      }
   }

   out.tokens.push_back(
      Token{.kind = TokenKind::End, .text = {}, .loc = cur.loc});
   return out;
}

} // namespace clanker

