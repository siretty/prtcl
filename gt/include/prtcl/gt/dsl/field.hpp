#pragma once

#include "common.hpp"

#include <initializer_list>
#include <prtcl/gt/nd_dtype.hpp>
#include <prtcl/gt/nd_shape.hpp>

#include <initializer_list>
#include <string>

namespace prtcl::gt::dsl {

enum class field_kind {
  global,
  uniform,
  varying,
};

struct field {
  std::string name;
  field_kind kind;
  nd_dtype dtype;
  nd_shape shape;
};

using field_expr = term_type<field>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

inline auto
gr_field(std::string name_, std::initializer_list<size_t> shape_ = {}) {
  return field_expr{{name_, field_kind::global, nd_dtype::real, shape_}};
}

inline auto
ur_field(std::string name_, std::initializer_list<size_t> shape_ = {}) {
  return field_expr{{name_, field_kind::uniform, nd_dtype::real, shape_}};
}

inline auto
vr_field(std::string name_, std::initializer_list<size_t> shape_ = {}) {
  return field_expr{{name_, field_kind::varying, nd_dtype::real, shape_}};
}

} // namespace prtcl::gt::dsl::language
