#ifndef PRTCL_LOG_HPP
#define PRTCL_LOG_HPP

#include "log/logger.hpp"

namespace prtcl::log {

Logger &GetLogger();

void SetLogger(Logger *logger);

template<typename ...Args>
Logger &Log(Level level, typename Logger::StringView target,
    typename Logger::StringView origin, Args &&...args) {
  return GetLogger().Log(level, target, origin, std::forward<Args>(args)...);
}

template<typename ...Args>
Logger &
Debug(typename Logger::StringView target, typename Logger::StringView origin,
    Args &&...args) {
  return Log(Level::kDebug, target, origin, std::forward<Args>(args)...);
}

template<typename ...Args>
Logger &
Info(typename Logger::StringView target, typename Logger::StringView origin,
    Args &&...args) {
  return Log(Level::kInfo, target, origin, std::forward<Args>(args)...);
}

template<typename ...Args>
Logger &
Warning(typename Logger::StringView target, typename Logger::StringView origin,
    Args &&...args) {
  return Log(Level::kWarning, target, origin, std::forward<Args>(args)...);
}

template<typename ...Args>
Logger &
Error(typename Logger::StringView target, typename Logger::StringView origin,
    Args &&...args) {
  return Log(Level::kError, target, origin, std::forward<Args>(args)...);
}

} // namespace prtcl::log

#endif //PRTCL_LOG_HPP
