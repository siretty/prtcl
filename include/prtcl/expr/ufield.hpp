#pragma once

#include <prtcl/tag/kind.hpp>
#include <prtcl/tag/type.hpp>

#include <string>

namespace prtcl::expr {

template <typename TT, typename V> struct ufield {
  static_assert(tag::is_type_v<TT>);

  using kind_tag = tag::kind::uniform;
  using type_tag = TT;

  V value;
};

inline namespace literals {

ufield<tag::type::scalar, std::string> operator""_us(char const *ptr,
                                                     size_t len) {
  return {std::string{ptr, len}};
}

ufield<tag::type::vector, std::string> operator""_uv(char const *ptr,
                                                     size_t len) {
  return {std::string{ptr, len}};
}

ufield<tag::type::matrix, std::string> operator""_um(char const *ptr,
                                                     size_t len) {
  return {std::string{ptr, len}};
}

} // namespace literals

} // namespace prtcl::expr
