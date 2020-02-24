#pragma once

#include <ostream>
#include <string>

namespace prtcl::gt {

// {{{ implementation details
namespace details {

template <typename CharT_, typename Traits_>
struct cxx_escape_double_quotes_sm_type {
  friend std::basic_ostream<CharT_, Traits_> &operator<<(
      std::basic_ostream<CharT_, Traits_> &stream_,
      cxx_escape_double_quotes_sm_type const &self_) {
    for (auto ch : self_.text) {
      switch (ch) {
      case '"':
      case '\\':
        stream_ << '\\';
      }
      stream_ << ch;
    }
    return stream_;
  }

  std::string text;
};

} // namespace details
// }}}

template <typename CharT_, typename Traits_>
static auto cxx_escape_double_quotes(std::basic_string<CharT_, Traits_> text_) {
  return details::cxx_escape_double_quotes_sm_type<CharT_, Traits_>{text_};
}

template <typename CharT_>
static auto cxx_escape_double_quotes(CharT_ const *text_) {
  return cxx_escape_double_quotes(std::basic_string<CharT_>{text_});
}

} // namespace prtcl::gt
