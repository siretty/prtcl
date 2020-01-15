#pragma once

#include "common.hpp"

#include <initializer_list>
#include <string>

namespace prtcl::gt::dsl {

enum class field_kind {
  global,
  uniform,
  varying,
};

template <field_kind Kind_> struct field {
  static constexpr field_kind kind = Kind_;

  std::string name;
  nd_dtype dtype;
  nd_shape shape;
};

template <field_kind Kind_> using field_expr = term_expr<field<Kind_>>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

inline auto
gr_field(std::string name_, std::initializer_list<size_t> shape_ = {}) {
  return field_expr<field_kind::global>{{name_, nd_dtype::real, shape_}};
}

inline auto
ur_field(std::string name_, std::initializer_list<size_t> shape_ = {}) {
  return field_expr<field_kind::uniform>{{name_, nd_dtype::real, shape_}};
}

inline auto
vr_field(std::string name_, std::initializer_list<size_t> shape_ = {}) {
  return field_expr<field_kind::varying>{{name_, nd_dtype::real, shape_}};
}

inline auto
gi_field(std::string name_, std::initializer_list<size_t> shape_ = {}) {
  return field_expr<field_kind::global>{{name_, nd_dtype::integer, shape_}};
}

inline auto
ui_field(std::string name_, std::initializer_list<size_t> shape_ = {}) {
  return field_expr<field_kind::uniform>{{name_, nd_dtype::integer, shape_}};
}

inline auto
vi_field(std::string name_, std::initializer_list<size_t> shape_ = {}) {
  return field_expr<field_kind::varying>{{name_, nd_dtype::integer, shape_}};
}

} // namespace prtcl::gt::dsl::language
