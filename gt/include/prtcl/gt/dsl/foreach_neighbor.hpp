#pragma once

#include "common.hpp"

namespace prtcl::gt::dsl {

struct foreach_neighbor {};

using foreach_neighbor_expr = term_expr<foreach_neighbor>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

template <typename... Exprs_> auto foreach_neighbor(Exprs_ &&... exprs_) {
  return foreach_neighbor_expr{}(std::forward<Exprs_>(exprs_)...);
}

} // namespace prtcl::gt::dsl::language
