#pragma once

#include <type_traits>

namespace prtcl::log {

enum class Level {
  kDebug = (1 << 1),
  kInfo = (1 << 2),
  kWarning = (1 << 3),
  kError = (1 << 4),
};

using LevelMask = std::underlying_type_t<Level>;

inline LevelMask ToMask(Level level) {
  return static_cast<LevelMask>(level);
}

} // namespace prtcl::log