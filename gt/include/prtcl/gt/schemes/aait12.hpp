#pragma once

#include <prtcl/gt/dsl.hpp>

namespace prtcl::gt::schemes::aait12 {

inline auto wavg_kernel() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const h = gr_field("smoothing_scale", {});
  auto const x = vr_field("position", {0});

  // fixed choice for R (description of figure 3b)
  auto const R = 2 * h;
  // directly evaluate s^2
  auto const s_sq = norm_squared(x[i] - x[j]) / (R * R);
  // compute the weighting kernel
  return max(0, (1 - s_sq) * (1 - s_sq) * (1 - s_sq));
}

// TODO: compute the correction factor f defined in eq. (3)
//       in order for this to work, we need the gradient of the weighted average
//       of neighboring particle positions wrt. the evaluation position

} // namespace prtcl::gt::schemes::aait12

// From: An Efficient Surface Reconstruction Pipeline for Particle-Based Fluids
