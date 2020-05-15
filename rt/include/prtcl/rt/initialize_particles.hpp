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
  using o = typename math_policy::operations;

  auto const group_type = group.get_type();
  auto const group_tags = core::make_set(group.tags());

  static constexpr size_t N = ModelPolicy_::dimensionality;

  auto const h = model.template get_global<dtype::real>("smoothing_scale");

  if (group_tags.contains("dynamic")) {
    auto const x = group.template add_varying<dtype::real, N>("position");
    auto const v = group.template add_varying<dtype::real, N>("velocity");
    auto const a = group.template add_varying<dtype::real, N>("acceleration");

    auto const t_b = group.template add_varying<dtype::real>("time_of_birth");

    for (auto const i : indices) {
      const auto j = static_cast<size_t>(i);

      x[j] = o::template zeros<dtype::real, N>();
      v[j] = o::template zeros<dtype::real, N>();
      a[j] = o::template zeros<dtype::real, N>();

      t_b[j] = o::template zeros<dtype::real>();
    }
  }

  if (group_type == "fluid") {
    // TODO: this is not the right place for setting a uniform
    auto const rho0 = group.template add_uniform<dtype::real>("rest_density");
    rho0[0] = 1000;

    auto const m = group.template add_varying<dtype::real>("mass");

    if (group_tags.contains("density")) {
      auto const rho = group.template add_varying<dtype::real>("density");

      for (auto const i : indices) {
        auto const j = static_cast<size_t>(i);

        m[j] = core::constpow(h[0], N) * rho0[0];
        rho[j] = 0;
      }
    }
  }

  if (group_type == "boundary") {
    auto const V = group.template add_varying<dtype::real>("volume");

    for (auto const i : indices) {
      auto const j = static_cast<size_t>(i);

      V[j] = 0;
    }
  }
}

} // namespace prtcl::rt
