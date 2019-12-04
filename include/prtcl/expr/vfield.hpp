#pragma once

#include <prtcl/tag/kind.hpp>
#include <prtcl/tag/type.hpp>

#include <string>

namespace prtcl::expr {

template <typename TT, typename V> struct vfield {
  static_assert(tag::is_type_v<TT>);

  using kind_tag = tag::kind::varying;
  using type_tag = TT;

  V value;
};

inline namespace literals {

vfield<tag::type::scalar, std::string> operator""_vs(char const *ptr,
                                                     size_t len) {
  return {std::string{ptr, len}};
}

vfield<tag::type::vector, std::string> operator""_vv(char const *ptr,
                                                     size_t len) {
  return {std::string{ptr, len}};
}

vfield<tag::type::matrix, std::string> operator""_vm(char const *ptr,
                                                     size_t len) {
  return {std::string{ptr, len}};
}

} // namespace literals

} // namespace prtcl::expr
