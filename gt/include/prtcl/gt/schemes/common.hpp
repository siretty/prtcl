#pragma once

#include <prtcl/gt/dsl.hpp>

namespace prtcl::gt::schemes::common {

inline auto fluid_fluid_density() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const x = vr_field("position", {0});

  auto const m = vr_field("mass", {});

  return m[j] * W(x[i] - x[j]);
}

inline auto fluid_fluid_pressure_acceleration() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const x = vr_field("position", {0});

  auto const m = vr_field("mass", {});
  auto const rho = vr_field("density", {});
  auto const p = vr_field("pressure", {});

  return m[j] * (p[i] / (rho[i] * rho[i]) + p[j] / (rho[j] * rho[j])) *
         dW(x[i] - x[j]);
}

inline auto fluid_fluid_viscosity_acceleration() {
  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;
  using namespace prtcl::gt::dsl::kernel_shorthand;

  auto const h = gr_field("smoothing_scale", {});

  auto const x = vr_field("position", {0});
  auto const v = vr_field("velocity", {0});

  auto const m = vr_field("mass", {});
  auto const rho = vr_field("density", {});
  auto const p = vr_field("pressure", {});

  auto const nu = ur_field("viscosity", {});

  return (nu[i] * (m[j] / rho[j]) * dot(v[i] - v[j], x[i] - x[j]) /
          (norm_squared(x[i] - x[j]) + 0.01 * h * h)) *
         dW(x[i] - x[j]);
}

} // namespace prtcl::gt::schemes::common
