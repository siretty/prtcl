#pragma once

#include "prtcl/gt/dsl/deep_copy.hpp"
#include <prtcl/gt/dsl.hpp>

namespace prtcl::gt::schemes::common {

inline auto f_f_density() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const x = vr_field("position", {0});
  auto const m = vr_field("mass", {});

  return dsl::deep_copy( //
      m[j] * W(x[i] - x[j]));
}

inline auto f_pressure_state_equation() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const k = ur_field("compressibility", {});
  auto const rho0 = ur_field("rest_density", {});

  auto const rho = vr_field("density", {});

  return dsl::deep_copy( //
      k[i] * max(0.0, rho[i] / rho0[i] - 1.0));
}

inline auto f_f_pressure_acceleration() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const x = vr_field("position", {0});
  auto const m = vr_field("mass", {});
  auto const p = vr_field("pressure", {});
  auto const rho = vr_field("density", {});

  return dsl::deep_copy( //
      m[j] * (p[i] / (rho[i] * rho[i]) + p[j] / (rho[j] * rho[j])) *
      dW(x[i] - x[j]));
}

inline auto f_f_viscosity_acceleration() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const h = gr_field("smoothing_scale", {});

  auto const nu = ur_field("viscosity", {});

  auto const x = vr_field("position", {0});
  auto const v = vr_field("velocity", {0});
  auto const m = vr_field("mass", {});
  auto const p = vr_field("pressure", {});
  auto const rho = vr_field("density", {});

  return dsl::deep_copy( //
      (nu[i] * (m[j] / rho[j]) * dot(v[i] - v[j], x[i] - x[j]) /
       (norm_squared(x[i] - x[j]) + 0.01 * h * h)) *
      dW(x[i] - x[j]));
}

} // namespace prtcl::gt::schemes::common