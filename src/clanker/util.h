#pragma once

#include <optional>
#include <string>
#include <vector>

namespace clanker {

std::string trim(const std::string& s);
std::string rtrim(const std::string& s);
bool ends_with(const std::string& s, const std::string& suffix);

std::vector<std::string> split_ws(const std::string& s);
std::vector<std::string> split_lines(const std::string& s);

std::optional<int> to_int(const std::string& s);

} // namespace clanker

