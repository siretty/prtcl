#ifndef PRTCL_OSTREAM_LOGGER_HPP
#define PRTCL_OSTREAM_LOGGER_HPP

#include "logger.hpp"

namespace prtcl::log {

class OStreamLogger : public Logger {
public:
  Logger &
  LogImpl(Duration when, Level level, StringView target, StringView origin,
      StringView message) override {
    // begin coloring if enabled
    if (NeedsColor())
      (*ostream_) << GetColorPrefix(level);
    // format the log level
    switch (level) {
      case Level::kDebug:
        (*ostream_) << "D ";
        break;
      case Level::kInfo:
        (*ostream_) << "I ";
        break;
      case Level::kWarning:
        (*ostream_) << "W ";
        break;
      case Level::kError:
        (*ostream_) << "E ";
        break;
      default:
        (*ostream_) << "? ";
        break;
    }
    // format the rest of the message
    (*ostream_) << '[' << when.count() << '|' << target << '|' << origin << "]";
    // end the coloring if enabled
    if (NeedsColor())
      (*ostream_) << GetColorSuffix(level);
    // output the message
    (*ostream_) << ' ' << message << '\n';
    return *this;
  }

public:
  explicit OStreamLogger(std::basic_ostream<CharType, CharTraits> *ostream,
      bool _delete = true) : ostream_{ostream}, delete_{_delete} {}

  ~OStreamLogger() override {
    if (delete_ and ostream_) {
      delete ostream_;
    }
    ostream_ = nullptr;
  }

private:
  bool NeedsColor() const;

  StringView GetColorPrefix(Level level) const;

  StringView GetColorSuffix(Level level) const;

private:
  std::basic_ostream<CharType, CharTraits> *ostream_;
  bool delete_;
};

} // namespace prtcl::log

#endif //PRTCL_OSTREAM_LOGGER_HPP
