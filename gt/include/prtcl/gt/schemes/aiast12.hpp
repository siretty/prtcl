#pragma once

#include "prtcl/gt/dsl/deep_copy.hpp"
#include <prtcl/gt/dsl.hpp>

// Title:   Versatile Rigid-Fluid Coupling for Incompressible SPH
// Year:    2012
// Authors: Nadir Akinci, Markus Ihmsen, Gizem Akinci, Barbara Solenthaler,
//          Matthias Teschner

namespace prtcl::gt::schemes::aiast12 {

inline auto f_b_density() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const rho0 = ur_field("rest_density", {});

  auto const x = vr_field("position", {0});
  auto const V = vr_field("volume", {});

  return dsl::deep_copy( //
      V[j] * rho0[i] * W(x[i] - x[j]));
}

inline auto f_b_pressure_acceleration() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const rho0 = ur_field("rest_density", {});

  auto const x = vr_field("position", {0});
  auto const p = vr_field("pressure", {});
  auto const V = vr_field("volume", {});
  auto const rho = vr_field("density", {});

  return dsl::deep_copy( //
      0.7 * V[j] * rho0[i] * (2 * p[i] / (rho[i] * rho[i])) * dW(x[i] - x[j]));
}

inline auto f_b_viscosity_acceleration() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const h = gr_field("smoothing_scale", {});

  auto const nu = ur_field("viscosity", {});

  auto const x = vr_field("position", {0});
  auto const v = vr_field("velocity", {0});
  auto const V = vr_field("volume", {});
  auto const rho = vr_field("density", {});

  return dsl::deep_copy( //
      (nu[i] * V[j] * dot(v[i], x[i] - x[j]) /
       (norm_squared(x[i] - x[j]) + 0.01 * h * h)) *
      dW(x[i] - x[j]));
}

} // namespace prtcl::gt::schemes::aiast12
