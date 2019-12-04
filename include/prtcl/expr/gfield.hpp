#pragma once

#include <prtcl/tag/kind.hpp>
#include <prtcl/tag/type.hpp>

#include <string>
#include <type_traits>

namespace prtcl::expr {

template <typename TT> struct gfield {
  static_assert(tag::is_type_v<TT>);

  using kind_tag = tag::kind::global;
  using type_tag = TT;

  std::string value;
};

template <typename> struct is_gfield : std::false_type {};

inline namespace literals {

gfield<tag::type::scalar> operator""_gs(char const *ptr, size_t len) {
  return {std::string{ptr, len}};
}

gfield<tag::type::vector> operator""_gv(char const *ptr, size_t len) {
  return {std::string{ptr, len}};
}

gfield<tag::type::matrix> operator""_gm(char const *ptr, size_t len) {
  return {std::string{ptr, len}};
}

} // namespace literals

} // namespace prtcl::expr
