#pragma once

#include "basic_group.hpp"
#include "basic_model.hpp"

#include <prtcl/core/constpow.hpp>
#include <prtcl/core/make_set.hpp>

namespace prtcl::rt {

template <typename ModelPolicy_, typename IndexRange_>
void initialize_particles(
    basic_model<ModelPolicy_> const &model, basic_group<ModelPolicy_> &group,
    IndexRange_ indices) {
  using math_policy = typename ModelPolicy_::math_policy;

  using c = typename math_policy::constants;

  auto const group_type = group.get_type();
  auto const group_tags = core::make_set(group.tags());

  static constexpr size_t N = ModelPolicy_::dimensionality;

  auto const h = model.template get_global<nd_dtype::real>("smoothing_scale");

  if (group_tags.contains("dynamic")) {
    auto const x = group.template add_varying<nd_dtype::real, N>("position");
    auto const v = group.template add_varying<nd_dtype::real, N>("velocity");
    auto const a =
        group.template add_varying<nd_dtype::real, N>("acceleration");

    auto const t_b =
        group.template add_varying<nd_dtype::real>("time_of_birth");

    for (auto const i : indices) {
      x[i] = c::template zeros<nd_dtype::real, N>();
      v[i] = c::template zeros<nd_dtype::real, N>();
      a[i] = c::template zeros<nd_dtype::real, N>();

      t_b[i] = c::template zeros<nd_dtype::real>();
    }
  }

  if (group_type == "fluid") {
    auto const rho0 =
        group.template add_uniform<nd_dtype::real>("rest_density");
    rho0[0] = 1000;

    auto const m = group.template add_varying<nd_dtype::real>("mass");

    if (group_tags.contains("density")) {
      auto const rho = group.template add_varying<nd_dtype::real>("density");

      for (auto const i : indices) {
        m[i] = core::constpow(h[0], N) * rho0[0];
        rho[i] = 0;
      }
    }
  }

  if (group_type == "boundary") {
    auto const V = group.template add_varying<nd_dtype::real>("volume");

    for (auto const i : indices) {
      V[i] = 0;
    }
  }
}

} // namespace prtcl::rt
