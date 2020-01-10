#pragma once

#include "common.hpp"

namespace prtcl::gt::dsl {

struct particle_index {};

using particle_index_expr = term_expr<particle_index>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::monaghan_indices {

static auto const a = particle_index_expr{};

} // namespace prtcl::gt::dsl::monaghan_indices

namespace prtcl::gt::dsl::generic_indices {

static auto const i = particle_index_expr{};

} // namespace prtcl::gt::dsl::generic_indices
