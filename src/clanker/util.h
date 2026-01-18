// src/clanker/util.h

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace clanker {

std::string trim(const std::string& s);
std::string rtrim(const std::string& s);
bool ends_with(const std::string& s, const std::string& suffix);

std::vector<std::string> split_ws(const std::string& s);
std::vector<std::string> split_lines(const std::string& s);

std::optional<int> to_int(const std::string& s);

// Low-level write helper for fd-backed builtins.
// Returns false on error (other than EINTR, which is retried).
bool fd_write_all(int fd, std::string_view s) noexcept;

} // namespace clanker

