// src/clanker/security_policy.h

#pragma once

#include <cstdlib>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>

namespace clanker {

struct IdentitySnapshot {
   uid_t uid{};
   uid_t euid{};
   gid_t gid{};
   gid_t egid{};
};

inline IdentitySnapshot snapshot_identity() noexcept {
   return IdentitySnapshot{::getuid(), ::geteuid(), ::getgid(), ::getegid()};
}

class SecurityPolicy {
 public:
   static SecurityPolicy capture_startup_identity() noexcept {
      return SecurityPolicy{snapshot_identity()};
   }

   // Call once very early. Returns exit code (0 means OK).
   int refuse_root_start() const noexcept {
      if (start_.uid == 0 || start_.euid == 0) return 125;
      return 0;
   }

   // Call at execution boundaries.
   bool identity_unchanged() const noexcept {
      const auto now = snapshot_identity();
      return now.uid == start_.uid && now.euid == start_.euid &&
             now.gid == start_.gid && now.egid == start_.egid;
   }

 private:
   explicit SecurityPolicy(IdentitySnapshot start) noexcept
      : start_(start) {}

   IdentitySnapshot start_{};
};

} // namespace clanker

