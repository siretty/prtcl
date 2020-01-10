#pragma once

#include "common.hpp"

namespace prtcl::gt::dsl {

struct neighbor_index {};

using neighbor_index_expr = term_expr<neighbor_index>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::monaghan_indices {

static auto const b = neighbor_index_expr{};

} // namespace prtcl::gt::dsl::monaghan_indices

namespace prtcl::gt::dsl::generic_indices {

static auto const j = neighbor_index_expr{};

} // namespace prtcl::gt::dsl::generic_indices
