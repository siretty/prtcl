#pragma once

#include "common.hpp"

#include <prtcl/gt/nd_index.hpp>

namespace prtcl::gt::dsl {

struct component_index {
  nd_index index;
};

using component_index_expr = term_expr<component_index>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

inline auto operator""_idx(unsigned long long value_) {
  return component_index_expr{{{value_}}};
}

} // namespace prtcl::gt::dsl::language
