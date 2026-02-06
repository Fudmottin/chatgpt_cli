// src/clanker/ast.h
#pragma once

#include <optional>
#include <string>
#include <vector>

namespace clanker {

enum class RedirKind {
   In,        // <
   OutTrunc,  // >
   OutAppend, // >>
};

struct Redirection {
   int fd = -1;            // default 0 for <, default 1 for > / >>
   RedirKind kind{};
   std::string target;     // filename (WORD)
};

struct SimpleCommand {
   std::vector<std::string> argv;
   std::vector<Redirection> redirs;
};

struct Pipeline {
   std::vector<SimpleCommand> stages; // size >= 1
};

enum class AndOrOp {
   AndIf, // &&
   OrIf,  // ||
};

struct AndOrTail {
   AndOrOp op;
   Pipeline rhs;
};

struct AndOr {
   Pipeline first;
   std::vector<AndOrTail> rest; // each tail is (op, rhs)
};

enum class Terminator {
   None,      // no explicit terminator (end of input)
   Semicolon, // ;
   Newline,   // NEWLINE
   Ampersand, // &
};

struct CommandListItem {
   AndOr cmd;
   Terminator term; // terminator that followed this command
};

struct CommandList {
   std::vector<CommandListItem> items;
   std::optional<Terminator>
      trailing; // present only if input ends with ;, NEWLINE, or &
};

} // namespace clanker

