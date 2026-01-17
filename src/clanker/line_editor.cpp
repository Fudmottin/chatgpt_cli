#include "clanker/line_editor.h"

#include <iostream>

namespace clanker {

std::optional<std::string> LineEditor::readline(const std::string& prompt) {
   std::cout << prompt << std::flush;

   std::string line;
   if (!std::getline(std::cin, line))
      return std::nullopt;

   return line;
}

} // namespace clanker

