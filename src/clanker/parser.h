// src/clanker/parser.h
#pragma once

#include "clanker/ast.h"
#include <string>

namespace clanker {

enum class ParseKind { Complete, Incomplete, Error };

struct ParseResult {
   ParseKind kind{ParseKind::Error};
   Pipeline pipeline;     // valid when Complete
   std::string message;   // valid when Error
};

class Parser {
public:
   ParseResult parse(const std::string& input) const;
};

} // namespace clanker

