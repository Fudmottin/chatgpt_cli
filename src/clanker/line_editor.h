#pragma once

#include <optional>
#include <string>

namespace clanker {

class LineEditor {
public:
   std::optional<std::string> readline(const std::string& prompt);
};

} // namespace clanker

