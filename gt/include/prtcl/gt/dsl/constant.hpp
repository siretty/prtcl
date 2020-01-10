#pragma once

#include "common.hpp"

#include <initializer_list>
#include <prtcl/gt/nd_dtype.hpp>
#include <prtcl/gt/nd_shape.hpp>

#include <initializer_list>
#include <string>

namespace prtcl::gt::dsl {

struct constant {
  std::string name;
  nd_dtype dtype;
  nd_shape shape;
};

using constant_expr = term_expr<constant>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

inline auto zeros(std::initializer_list<size_t> shape_ = {}) {
  return constant_expr{{"zeros", nd_dtype::real, shape_}};
}

inline auto ones(std::initializer_list<size_t> shape_ = {}) {
  return constant_expr{{"ones", nd_dtype::real, shape_}};
}

inline auto identity(size_t extent_ = 0) {
  return constant_expr{{"identity", nd_dtype::real, {extent_, extent_}}};
}

static auto const particle_count =
    constant_expr{{"particle_count", nd_dtype::integer, {}}};

static auto const neighbor_count =
    constant_expr{{"neighbor_count", nd_dtype::integer, {}}};

} // namespace prtcl::gt::dsl::language
