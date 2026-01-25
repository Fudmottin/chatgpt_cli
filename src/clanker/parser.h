// src/clanker/parser.h
#pragma once

#include <string>
#include <vector>

#include "clanker/ast.h"

namespace clanker {

enum class ParseKind { Complete, Incomplete, Error };

struct ParseResult {
   ParseKind kind{ParseKind::Error};

   // Backwards-compatible: legacy callers can keep reading `pipeline`.
   // When parsing a sequential list, `list` is populated and `pipeline`
   // is typically left default-constructed (or can be ignored).
   Pipeline pipeline; // valid when Complete and result_is_pipeline()
   CommandList list;  // valid when Complete and result_is_list()

   std::string message; // valid when Error

   [[nodiscard]] bool result_is_pipeline() const noexcept {
      return kind == ParseKind::Complete && list.pipelines.empty();
   }

   [[nodiscard]] bool result_is_list() const noexcept {
      return kind == ParseKind::Complete && !list.pipelines.empty();
   }
};

class Parser {
 public:
   [[nodiscard]] ParseResult parse(const std::string& input) const;
};

} // namespace clanker

