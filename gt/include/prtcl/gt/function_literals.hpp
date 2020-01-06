#pragma once

#include <prtcl/gt/function.hpp>

#include <boost/yap/yap.hpp>

namespace prtcl::gt::function_literals {

inline auto operator""_fun(char const *ptr, size_t len) {
  return function{std::string_view{ptr, len}};
}

inline auto operator""_fun_term(char const *ptr, size_t len) {
  return ::boost::yap::make_terminal(function{std::string_view{ptr, len}});
}

} // namespace prtcl::gt::function_literals
