// src/clanker/parser.cpp
#include <cctype>
#include <optional>
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

   auto stage_empty = [](const SimpleCommand& st) {
      return st.argv.empty() && st.redirs.empty();
   };

   if (pl.stages.size() == 1 && stage_empty(pl.stages[0])) return true;

   for (const auto& st : pl.stages) {
      if (!stage_empty(st)) return false;
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
   case TokenKind::RedirectIn:
      return "<";
   case TokenKind::RedirectOut:
      return ">";
   case TokenKind::RedirectAppend:
      return ">>";
   case TokenKind::IoNumber:
      return "io-number";
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

static int default_redir_fd(TokenKind k) noexcept {
   return (k == TokenKind::RedirectIn) ? 0 : 1;
}

static RedirKind token_to_redir_kind(TokenKind k) noexcept {
   switch (k) {
   case TokenKind::RedirectIn:
      return RedirKind::In;
   case TokenKind::RedirectOut:
      return RedirKind::OutTrunc;
   case TokenKind::RedirectAppend:
      return RedirKind::OutAppend;
   default:
      return RedirKind::OutTrunc; // unreachable for valid callers
   }
}

static Terminator token_to_terminator(TokenKind k) {
   switch (k) {
   case TokenKind::Semicolon:
      return Terminator::Semicolon;
   case TokenKind::Newline:
      return Terminator::Newline;
   case TokenKind::Ampersand:
      return Terminator::Ampersand;
   default:
      return Terminator::None;
   }
}

static AndOrOp token_to_andor_op(TokenKind k) {
   switch (k) {
   case TokenKind::AndIf:
      return AndOrOp::AndIf;
   case TokenKind::OrIf:
      return AndOrOp::OrIf;
   default:
      // Should never be called with other tokens.
      return AndOrOp::AndIf;
   }
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
   // Trailing ';' / NEWLINE / '&' is complete (it terminates a list element).
   if (is_trailing_control_operator(lr)) return {.kind = ParseKind::Incomplete};

   CommandList list;

   Pipeline current;
   current.stages.push_back(SimpleCommand{});

   auto reset_current = [&] {
      current = Pipeline{};
      current.stages.push_back(SimpleCommand{});
   };

   AndOr current_andor;
   bool has_andor_first = false;
   std::optional<AndOrOp> pending_andor_op;

   auto reset_andor = [&] {
      current_andor = AndOr{};
      has_andor_first = false;
      pending_andor_op.reset();
   };

   auto validate_current_pipeline_complete = [&]() -> ParseResult {
      if (pipeline_is_empty(current)) return {.kind = ParseKind::Complete};

      if (!current.stages.empty() && current.stages.back().argv.empty() &&
          current.stages.back().redirs.empty()) {
         return parse_error("syntax error: empty pipeline stage");
      }

      return {.kind = ParseKind::Complete};
   };

   auto commit_current_pipeline_into_andor = [&]() -> ParseResult {
      if (pipeline_is_empty(current)) {
         // Nothing to commit.
         reset_current();
         return {.kind = ParseKind::Complete};
      }

      const ParseResult v = validate_current_pipeline_complete();
      if (v.kind == ParseKind::Error) return v;

      Pipeline pl = std::move(current);
      reset_current();

      if (!has_andor_first) {
         current_andor.first = std::move(pl);
         has_andor_first = true;
         return {.kind = ParseKind::Complete};
      }

      if (!pending_andor_op.has_value()) {
         return parse_error(
            "syntax error: missing '&&' or '||' between pipelines");
      }

      current_andor.rest.push_back(
         AndOrTail{.op = *pending_andor_op, .rhs = std::move(pl)});
      pending_andor_op.reset();
      return {.kind = ParseKind::Complete};
   };

   auto flush_andor_to_list = [&](Terminator term) -> ParseResult {
      // If there's a non-empty pipeline pending, commit it first.
      const ParseResult c = commit_current_pipeline_into_andor();
      if (c.kind == ParseKind::Error) return c;

      if (pending_andor_op.has_value()) {
         return parse_error("syntax error: trailing control operator");
      }

      if (!has_andor_first) {
         // Nothing to flush; this is just a trailing terminator.
         list.trailing = term;
         return {.kind = ParseKind::Complete};
      }

      list.items.push_back(
         CommandListItem{.cmd = std::move(current_andor), .term = term});
      reset_andor();
      list.trailing.reset();
      return {.kind = ParseKind::Complete};
   };

   std::optional<int> pending_fd;

   for (std::size_t i = 0; i < lr.tokens.size(); ++i) {
      const Token& t = lr.tokens[i];

      switch (t.kind) {
      case TokenKind::Word:
         current.stages.back().argv.push_back(t.text);
         break;

      case TokenKind::IoNumber: {
         int fd = 0;
         for (char ch : t.text) {
            if (!std::isdigit(static_cast<unsigned char>(ch)))
               return parse_error("syntax error: invalid io-number");
            fd = fd * 10 + (ch - '0');
         }
         pending_fd = fd;
         break;
      }

      case TokenKind::RedirectIn:
      case TokenKind::RedirectOut:
      case TokenKind::RedirectAppend: {
         if (i + 1 >= lr.tokens.size())
            return parse_error("syntax error: expected redirection target");

         const Token& target = lr.tokens[i + 1];
         if (target.kind != TokenKind::Word)
            return parse_error("syntax error: expected redirection target");

         const int fd =
            pending_fd.value_or((t.kind == TokenKind::RedirectIn) ? 0 : 1);
         pending_fd.reset();

         RedirKind rk{};
         if (t.kind == TokenKind::RedirectIn)
            rk = RedirKind::In;
         else if (t.kind == TokenKind::RedirectOut)
            rk = RedirKind::OutTrunc;
         else
            rk = RedirKind::OutAppend;

         current.stages.back().redirs.push_back(
            Redirection{.fd = fd, .kind = rk, .target = target.text});

         ++i; // consume target
         break;
      }

      case TokenKind::Pipe:
         if (pending_fd.has_value())
            return parse_error("syntax error: io-number without redirection");
         if (current.stages.back().argv.empty() &&
             current.stages.back().redirs.empty()) {
            return parse_error("syntax error: empty pipeline stage before '|'");
         }
         current.stages.push_back(SimpleCommand{});
         break;

      case TokenKind::AndIf:
      case TokenKind::OrIf: {
         if (pending_fd.has_value())
            return parse_error("syntax error: io-number without redirection");
         const ParseResult c = commit_current_pipeline_into_andor();
         if (c.kind == ParseKind::Error) return c;

         if (!has_andor_first) {
            return parse_error(std::string("syntax error: operator '") +
                               token_spelling(t.kind) +
                               "' without left operand");
         }
         if (pending_andor_op.has_value())
            return parse_error("syntax error: consecutive control operators");

         pending_andor_op = token_to_andor_op(t.kind);
         break;
      }

      case TokenKind::Semicolon:
      case TokenKind::Newline:
      case TokenKind::Ampersand: {
         if (pending_fd.has_value())
            return parse_error("syntax error: io-number without redirection");
         const Terminator term = token_to_terminator(t.kind);
         const ParseResult f = flush_andor_to_list(term);
         if (f.kind == ParseKind::Error) return f;
         break;
      }

      case TokenKind::End:
         if (pending_fd.has_value())
            return parse_error("syntax error: io-number without redirection");
         break;
      }
   }

   // Final flush at EOF (no terminator).
   {
      const ParseResult f = flush_andor_to_list(Terminator::None);
      if (f.kind == ParseKind::Error) return f;
   }

   // Empty input (or comment-only): keep legacy behavior.
   if (list.items.empty()) {
      return {.kind = ParseKind::Complete, .pipeline = Pipeline{}};
   }

   // Backward compatibility: single simple pipeline with no terminator and no
   // &&/||
   if (list.items.size() == 1 && list.items[0].term == Terminator::None &&
       list.items[0].cmd.rest.empty()) {
      Pipeline pl = std::move(list.items[0].cmd.first);
      return {.kind = ParseKind::Complete, .pipeline = std::move(pl)};
   }

   return {.kind = ParseKind::Complete, .list = std::move(list)};
}

} // namespace clanker

