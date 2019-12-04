#pragma once

#include <prtcl/tag/kind.hpp>
#include <prtcl/tag/type.hpp>

namespace prtcl::expr {

template <typename KT, typename TT, typename V> struct field_base {
  using kind_tag_type = KT;
  using type_tag_type = TT;
  using value_type = V;

  static constexpr kind_tag_type kind_tag = {};
  static constexpr type_tag_type type_tag = {};

  V value;
};

} // namespace prtcl::expr
