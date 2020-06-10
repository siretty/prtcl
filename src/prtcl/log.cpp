#include <prtcl/log.hpp>

#include <prtcl/log/ostream_logger.hpp>

#include <iostream>

#include <cassert>

namespace prtcl::log {

namespace {

Logger *&GetLoggerImpl() {
  using CharType = typename Logger::CharType;
  using CharTraits = typename Logger::CharTraits;
  using BasicOStream = typename std::basic_ostream<CharType, CharTraits>;

  static Logger *logger = new OStreamLogger{
      static_cast<BasicOStream *>(&std::cerr), false};

  return logger;
}

} // namespace

Logger &GetLogger() {
  return *GetLoggerImpl();
}

void SetLogger(Logger *logger) {
  assert(logger != nullptr);
  delete GetLoggerImpl();
  GetLoggerImpl() = logger;
}

} // namespace prtcl
