#pragma once

#include "common.hpp"

#include <prtcl/gt/nd_index.hpp>

namespace prtcl::gt::dsl {

struct component_index {
  nd_index index;
};

using component_index_expr = term_type<component_index>;

} // namespace prtcl::gt::dsl
