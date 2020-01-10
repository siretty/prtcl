#pragma once

#include "common.hpp"

namespace prtcl::gt::dsl {

struct foreach_particle {};

using foreach_particle_expr = term_type<foreach_particle>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

template <typename... Exprs_> auto foreach_particle(Exprs_ &&... exprs_) {
  return foreach_particle_expr{}(std::forward<Exprs_>(exprs_)...);
}

} // namespace prtcl::gt::dsl::language
