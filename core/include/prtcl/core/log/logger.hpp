#pragma once

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace prtcl::core::log {

template <typename, typename> class basic_logger;

using logger = basic_logger<char, std::char_traits<char>>;

enum class log_level { debug, info, error };

inline logger &get_logger();

// {{{ implementation details
namespace details {

inline std::chrono::high_resolution_clock::time_point get_start_time() {
  static std::chrono::high_resolution_clock::time_point start_time =
      std::chrono::high_resolution_clock::now();
  return start_time;
}

inline std::chrono::high_resolution_clock::time_point get_current_time() {
  return std::chrono::high_resolution_clock::now();
}

} // namespace details
// }}}

template <
    typename CharType_, typename CharTraits_ = std::char_traits<CharType_>>
struct log_raii {
  using string = std::basic_string<CharType_, CharTraits_>;
  using string_view = std::basic_string_view<CharType_, CharTraits_>;

  log_raii(
      log_level level_, string_view target_, string_view origin_,
      string_view message_,
      basic_logger<CharType_, CharTraits_> *logger_ = nullptr)
      : level{level_}, target{target_}, origin{origin_}, message{message_},
        logger{logger_} {
    (logger ? *logger : get_logger())
        .log(level, target, origin, "[[[ " + message);
  }

  ~log_raii() {
    (logger ? *logger : get_logger())
        .log(level, target, origin, "]]] " + message);
  }

  log_level level;
  string target;
  string origin;
  string message;
  basic_logger<CharType_, CharTraits_> *logger;
};

template <
    typename CharType_, typename CharTraits_ = std::char_traits<CharType_>>
class basic_logger {
public:
  using string_view = std::basic_string_view<CharType_, CharTraits_>;

public:
  virtual ~basic_logger() {}

public:
  virtual basic_logger &
  log(log_level /* level */, string_view /* target */, string_view /* origin */,
      string_view /* message */) {
    return *this;
  }

public:
  template <typename... Args_>
  basic_logger &
  debug(string_view target, string_view origin, Args_ &&... args) {
    std::basic_stringstream<CharType_, CharTraits_> ss;
    (ss << ... << std::forward<Args_>(args));
    return log(log_level::debug, target, origin, ss.str());
  }

  template <typename... Args_>
  basic_logger &info(string_view target, string_view origin, Args_ &&... args) {
    std::basic_stringstream<CharType_, CharTraits_> ss;
    (ss << ... << std::forward<Args_>(args));
    return log(log_level::debug, target, origin, ss.str());
  }

  template <typename... Args_>
  basic_logger &
  error(string_view target, string_view origin, Args_ &&... args) {
    std::basic_stringstream<CharType_, CharTraits_> ss;
    (ss << ... << std::forward<Args_>(args));
    return log(log_level::debug, target, origin, ss.str());
  }

public:
  virtual log_raii<CharType_, CharTraits_> raii(
      log_level level_, string_view target_, string_view origin_,
      string_view message_) {
    return {level_, target_, origin_, message_, this};
  }

public:
  template <typename... Args_>
  auto debug_raii(string_view target, string_view origin, Args_ &&... args) {
    std::basic_stringstream<CharType_, CharTraits_> ss;
    (ss << ... << std::forward<Args_>(args));
    return raii(log_level::debug, target, origin, ss.str());
  }

  template <typename... Args_>
  auto info_raii(string_view target, string_view origin, Args_ &&... args) {
    std::basic_stringstream<CharType_, CharTraits_> ss;
    (ss << ... << std::forward<Args_>(args));
    return raii(log_level::debug, target, origin, ss.str());
  }

  template <typename... Args_>
  auto error_raii(string_view target, string_view origin, Args_ &&... args) {
    std::basic_stringstream<CharType_, CharTraits_> ss;
    (ss << ... << std::forward<Args_>(args));
    return raii(log_level::debug, target, origin, ss.str());
  }
};

using logger = basic_logger<char>;

template <
    typename CharType_, typename CharTraits_ = std::char_traits<CharType_>>
class basic_ostream_logger : public basic_logger<CharType_, CharTraits_> {
public:
  using basic_logger_type = basic_logger<CharType_, CharTraits_>;

private:
  using string_view = typename basic_logger_type::string_view;

public:
  basic_logger_type &
  log(log_level level, string_view target, string_view origin,
      string_view message) override {
    switch (level) {
    case log_level::debug:
      (*_ostream) << "D ";
      break;
    case log_level::info:
      (*_ostream) << "I ";
      break;
    case log_level::error:
      (*_ostream) << "E ";
      break;
    default:
      (*_ostream) << "? ";
      break;
    }
    (*_ostream)
        << '['
        << (details::get_current_time() - details::get_start_time()).count()
        << '|' << target << '|' << origin << "] " << message << std::endl;
    return *this;
  }

public:
  basic_ostream_logger(
      std::basic_ostream<CharType_, CharTraits_> *ostream_, bool delete_ = true)
      : _ostream{ostream_}, _delete{delete_} {}

  ~basic_ostream_logger() {
    if (_delete and _ostream) {
      delete _ostream;
    }
    _ostream = nullptr;
  }

private:
  std::basic_ostream<CharType_, CharTraits_> *_ostream;
  bool _delete;
};

using ostream_logger = basic_ostream_logger<char>;

// {{{ implementation details
namespace details {

inline logger *&get_the_logger() {
  // NOTE: This object stored here at termination time **will** leak. This is
  //       expected and part of the prtcl::rt::log interface.
  static logger *the_logger = new ostream_logger{&std::cerr, false};
  return the_logger;
}

} // namespace details
// }}}

// {{{ get_logger() -> logger &, set_logger(logger *)

inline logger &get_logger() { return *details::get_the_logger(); }

inline void set_logger(logger *logger_) {
  delete details::get_the_logger();
  details::get_the_logger() = logger_;
}

// }}}

template <typename... Args_>
inline logger &
debug(std::string_view target, std::string_view origin, Args_ &&... args) {
  return get_logger().debug(target, origin, std::forward<Args_>(args)...);
}

template <typename... Args_>
inline auto
debug_raii(std::string_view target, std::string_view origin, Args_ &&... args) {
  return get_logger().debug_raii(target, origin, std::forward<Args_>(args)...);
}

template <typename... Args_>
inline logger &
info(std::string_view target, std::string_view origin, Args_ &&... args) {
  return get_logger().info(target, origin, std::forward<Args_>(args)...);
}

template <typename... Args_>
inline auto
info_raii(std::string_view target, std::string_view origin, Args_ &&... args) {
  return get_logger().info_raii(target, origin, std::forward<Args_>(args)...);
}

template <typename... Args_>
inline logger &
error(std::string_view target, std::string_view origin, Args_ &&... args) {
  return get_logger().error(target, origin, std::forward<Args_>(args)...);
}

template <typename... Args_>
inline auto
error_raii(std::string_view target, std::string_view origin, Args_ &&... args) {
  return get_logger().error_raii(target, origin, std::forward<Args_>(args)...);
}

} // namespace prtcl::core::log
