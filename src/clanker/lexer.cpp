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

   char peek_n(std::size_t n) const noexcept {
      const std::size_t j = i + n;
      return (j >= s.size()) ? '\0' : s[j];
   }

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

   bool consume3(char a, char b, char c) noexcept {
      if (peek() != a) return false;
      if (peek_n(1) != b) return false;
      if (peek_n(2) != c) return false;
      advance();
      advance();
      advance();
      return true;
   }
};

} // namespace

bool Lexer::is_hspace(char c) noexcept {
   return c == ' ' || c == '\t' || c == '\r';
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

   auto skip_hspace = [&]() {
      while (!cur.eof() && is_hspace(cur.peek())) cur.advance();
   };

   auto skip_comment = [&]() {
      // assumes current char is '#'
      while (!cur.eof() && cur.peek() != '\n') cur.advance();
      // do NOT consume '\n' here; it is a terminator token
   };

   auto lex_word = [&]() -> LexResult {
      const SourceLoc start = cur.loc;
      std::string w;

      bool in_single = false;
      bool in_double = false;

      bool in_triple = false;
      char triple_q = '\0'; // '\'' or '"'

      bool in_backtick = false;

      int brace_depth = 0;
      int subst_paren_depth = 0; // for $(...)

      auto append = [&](char c) { w.push_back(c); };

      auto is_token_boundary = [&](char c) -> bool {
         if (in_single || in_double || in_triple || in_backtick) return false;
         if (brace_depth > 0 || subst_paren_depth > 0) return false;

         if (is_hspace(c)) return true;
         if (c == '\n') return true;
         if (c == ';') return true;
         if (c == '#') return true; // comment begins at token boundary
         if (c == '|') return true;
         if (c == '&') return true;
         if (c == '<') return true;
         if (c == '>') return true;

         return false;
      };

      auto lex_triple = [&](char q) -> LexResult {
         // current cursor points at the first quote of the triple sequence
         const SourceLoc qloc = cur.loc;
         if (q == '\'' && !cur.consume3('\'', '\'', '\'')) return {};
         if (q == '"' && !cur.consume3('"', '"', '"')) return {};
         // do not include the delimiters in the produced WORD; they affect
         // lexing only. If you prefer keeping them, append them here.
         in_triple = true;
         triple_q = q;
         (void)qloc;
         return LexResult{.kind = LexKind::Complete};
      };

      auto try_start_triple = [&]() -> bool {
         const char c = cur.peek();
         if (c != '\'' && c != '"') return false;
         if (cur.peek_n(1) != c) return false;
         if (cur.peek_n(2) != c) return false;
         LexResult r = lex_triple(c);
         if (r.kind == LexKind::Error) return true;
         if (r.kind == LexKind::Incomplete) return true;
         return true;
      };

      auto finish_triple_if_present = [&]() -> bool {
         if (!in_triple) return false;
         const char q = triple_q;
         if (cur.peek() != q) return false;
         if (cur.peek_n(1) != q) return false;
         if (cur.peek_n(2) != q) return false;
         cur.advance();
         cur.advance();
         cur.advance();
         in_triple = false;
         triple_q = '\0';
         return true;
      };

      auto try_start_command_subst = [&]() -> bool {
         // $(...)
         if (cur.peek() != '$') return false;
         if (cur.peek_n(1) != '(') return false;
         append('$');
         append('(');
         cur.advance();
         cur.advance();
         ++subst_paren_depth;
         return true;
      };

      auto try_start_backtick = [&]() -> bool {
         if (cur.peek() != '`') return false;
         // We do not include the delimiters in the WORD output, matching how
         // quotes are handled. If you prefer to preserve them, append them.
         cur.advance();
         in_backtick = true;
         return true;
      };

      auto finish_backtick_if_present = [&]() -> bool {
         if (!in_backtick) return false;
         if (cur.peek() != '`') return false;
         cur.advance();
         in_backtick = false;
         return true;
      };

      while (!cur.eof()) {
         const char c = cur.peek();

         // Triple-quoted body: everything is literal until matching delimiter.
         if (in_triple) {
            if (finish_triple_if_present()) continue;
            append(c);
            cur.advance();
            continue;
         }

         // Backtick body: allow escapes; terminate on unescaped backtick.
         if (in_backtick) {
            if (finish_backtick_if_present()) continue;

            if (c == '\\') {
               const SourceLoc esc_loc = cur.loc;
               cur.advance();
               if (cur.eof()) return incomplete_at(esc_loc);
               const char n = cur.peek();
               if (n == '\n') {
                  // continuation
                  cur.advance();
                  continue;
               }
               // bash/zsh-like: backslash escapes next char including '`'
               append(n);
               cur.advance();
               continue;
            }

            append(c);
            cur.advance();
            continue;
         }

         // Single-quoted body
         if (in_single) {
            if (c == '\'') {
               in_single = false;
               cur.advance();
               continue;
            }
            append(c);
            cur.advance();
            continue;
         }

         // Double-quoted body
         if (in_double) {
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
                  return error_at("unsupported escape in double quotes",
                                  esc_loc);
               }
               cur.advance();
               continue;
            }
            append(c);
            cur.advance();
            continue;
         }

         // Not in any quote-like mode.

         // Token boundary only when not inside brace-group or command-subst.
         if (is_token_boundary(c)) break;

         // Comments: only start a comment when at a token boundary. Since we
         // didn't break above, '#' here is not a boundary -> literal.
         // (E.g. foo#bar is a WORD.)

         // Triple quotes start (Python-style).
         if (try_start_triple()) {
            // started triple; do not include delimiters
            continue;
         }

         // Quotes
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

         // Command substitution: $(...)
         if (try_start_command_subst()) continue;

         // Backticks
         if (try_start_backtick()) continue;

         // Backslash escape (outside quotes)
         if (c == '\\') {
            const SourceLoc esc_loc = cur.loc;
            cur.advance();
            if (cur.eof()) return incomplete_at(esc_loc);
            const char n = cur.peek();
            if (n == '\n') {
               cur.advance();
               continue;
            }
            append(n);
            cur.advance();
            continue;
         }

         // Track brace-groups as lexical WORD constructs.
         if (c == '{') {
            ++brace_depth;
            append(c);
            cur.advance();
            continue;
         }
         if (c == '}') {
            if (brace_depth > 0) --brace_depth;
            append(c);
            cur.advance();
            continue;
         }

         // Track $(...) nesting by parentheses depth within substitution.
         if (subst_paren_depth > 0) {
            if (c == '(') {
               ++subst_paren_depth;
               append(c);
               cur.advance();
               continue;
            }
            if (c == ')') {
               --subst_paren_depth;
               append(c);
               cur.advance();
               continue;
            }
         }

         // Ordinary character
         append(c);
         cur.advance();
      }

      // If any construct is still open, we need more input.
      if (in_single || in_double || in_triple || in_backtick) {
         return incomplete_at(start);
      }
      if (brace_depth > 0 || subst_paren_depth > 0) return incomplete_at(start);

      if (w.empty()) return error_at("expected word", start);

      out.tokens.push_back(
         Token{.kind = TokenKind::Word, .text = std::move(w), .loc = start});
      return LexResult{.kind = LexKind::Complete};
   };

   while (!cur.eof()) {
      skip_hspace();
      if (cur.eof()) break;

      const char c = cur.peek();

      if (c == '\n') {
         const SourceLoc loc = cur.loc;
         cur.advance();
         push_op(TokenKind::Newline, loc);
         continue;
      }

      if (c == ';') {
         const SourceLoc loc = cur.loc;
         cur.advance();
         push_op(TokenKind::Semicolon, loc);
         continue;
      }

      if (c == '#') {
         skip_comment();
         continue;
      }

      // IO number: digits immediately followed by a redirection operator.
      // Example: 2>file, 12>>file
      if (c >= '0' && c <= '9') {
         const SourceLoc loc = cur.loc;

         std::size_t j = cur.i;
         while (j < cur.s.size()) {
            const char d = cur.s[j];
            if (d < '0' || d > '9') break;
            ++j;
         }

         const char next = (j < cur.s.size()) ? cur.s[j] : '\0';
         if (next == '<' || next == '>') {
            std::string digits;
            digits.reserve(j - cur.i);
            while (cur.i < j) {
               digits.push_back(cur.peek());
               cur.advance();
            }
            out.tokens.push_back(
               Token{.kind = TokenKind::IoNumber,
                     .text = std::move(digits),
                     .loc = loc});
            continue;
         }
         // Otherwise: fall through; it will lex as a WORD.
      }

      if (c == '<') {
         const SourceLoc loc = cur.loc;
         cur.advance();
         push_op(TokenKind::RedirectIn, loc);
         continue;
      }

      if (c == '>') {
         const SourceLoc loc = cur.loc;
         cur.advance();
         if (cur.consume('>')) {
            push_op(TokenKind::RedirectAppend, loc);
         } else {
            push_op(TokenKind::RedirectOut, loc);
         }
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

