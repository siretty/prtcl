#pragma once

#include <ostream>
#include <string>

namespace prtcl::gt {

// {{{ implementation details
namespace details {

template <typename CharT_, typename Traits_> struct cxx_inline_pragma_sm_type {
  friend std::basic_ostream<CharT_, Traits_> &operator<<(
      std::basic_ostream<CharT_, Traits_> &stream_,
      cxx_inline_pragma_sm_type const &self_) {
    return stream_ << "_Pragma(\"" << cxx_escape_double_quotes(self_.text)
                   << "\")";
  }

  std::string text;
};

} // namespace details
// }}}

template <typename CharT_, typename Traits_>
auto cxx_inline_pragma(std::basic_string<CharT_, Traits_> text_) {
  return details::cxx_inline_pragma_sm_type<CharT_, Traits_>{text_};
}

template <typename CharT_> auto cxx_inline_pragma(CharT_ const *text_) {
  return cxx_inline_pragma(std::basic_string<CharT_>{text_});
}

} // namespace prtcl::gt
