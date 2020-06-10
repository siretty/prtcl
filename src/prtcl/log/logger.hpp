#ifndef PRTCL_LOGGER_HPP
#define PRTCL_LOGGER_HPP

#include "level.hpp"

#include <string_view>
#include <sstream>
#include <chrono>

namespace prtcl::log {

class Logger {
public:
  using CharType = char;
  using CharTraits = std::char_traits<CharType>;
  using StringView = std::basic_string_view<CharType, CharTraits>;

protected:
  using Duration = std::chrono::high_resolution_clock::duration;

private:
  static auto GetCurrentTime() {
    return std::chrono::high_resolution_clock::now();
  }

  static auto GetStartTime() {
    static auto const start_time = GetCurrentTime();
    return start_time;
  }

public:
  virtual ~Logger() = default;

protected:
  virtual Logger &
  LogImpl(Duration when, Level level, StringView target, StringView origin,
      StringView message) {
    (void) (when), (void) (level), (void) (target), (void) (origin), (void) (message);
    return *this;
  }

public:
  Logger &LogMessage(Level level, StringView target, StringView origin,
      StringView message) {
    auto const start_time = GetStartTime();
    if (Enabled(level))
      return this->LogImpl(GetCurrentTime() - start_time, level, target, origin,
                           message);
    else
      return *this;
  }

  template<typename ...Args>
  Logger &
  Log(Level level, StringView target, StringView origin, Args &&...args) {
    if (Enabled(level)) {
      std::basic_ostringstream<CharType, CharTraits> ss;
      (ss << ... << std::forward<Args>(args));
      return LogMessage(level, target, origin, ss.str());
    } else
      return *this;
  }

public:
  bool Change(Level level, bool state = true) {
    if (state)
      level_mask_ |= ToMask(level);
    else
      level_mask_ &= ~ToMask(level);
    return state;
  }

  bool Toggle(Level level) { return Change(level, not Enabled(level)); }

  bool Enabled(Level level) const {
    return 0 != (level_mask_ & ToMask(level));
  }

private:
  LevelMask level_mask_ =
      ToMask(Level::kDebug) | ToMask(Level::kInfo) | ToMask(Level::kWarning) |
      ToMask(Level::kError);
};

} // namespace prtcl::log

#endif //PRTCL_LOGGER_HPP
