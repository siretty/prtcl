#include <prtcl/log/ostream_logger.hpp>

#include <iostream>

#if __has_include(<unistd.h>)
extern "C" {
#include <unistd.h>
} // extern "C"
#define IS_UNIX
#endif

namespace prtcl::log {

bool OStreamLogger::NeedsColor() const {
  #ifdef IS_UNIX
  if (ostream_ == &std::cout)
    return ::isatty(::fileno(::stdout));
  else if (ostream_ == &std::cerr)
    return ::isatty(::fileno(::stderr));
  else
    return false;
  #else
  return false;
  #endif
}

auto OStreamLogger::GetColorPrefix(Level level) const -> StringView {
  switch (level) {
    case Level::kDebug:
      return "\x1B[1;36m";
    case Level::kInfo:
      return "\x1B[1;37m";
    case Level::kWarning:
      return "\x1B[1;33m";
    case Level::kError:
      return "\x1B[1;31m";
  }
}

auto OStreamLogger::GetColorSuffix(Level level) const -> StringView {
  switch (level) {
    case Level::kDebug:
    case Level::kInfo:
    case Level::kWarning:
    case Level::kError:
      return "\x1B[0m";
  }
}

} //
