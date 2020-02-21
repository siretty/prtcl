#pragma once

#include <prtcl/core/remove_cvref.hpp>

#include <prtcl/rt/geometry/axis_aligned_box.hpp>
#include <prtcl/rt/integral_grid.hpp>
#include <prtcl/rt/triangle_mesh.hpp>

#include <array>
#include <type_traits>
#include <vector>

#include <cmath>

#include <boost/container/flat_set.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>

namespace prtcl::rt {

struct sample_volume_parameters {
  long double maximum_sample_distance;
};

template <typename ModelPolicy_, size_t N_, typename OutputIt_>
void sample_volume(
    axis_aligned_box<ModelPolicy_, N_> const &aab_, OutputIt_ it_,
    sample_volume_parameters const &p_) {
  using math_policy = typename ModelPolicy_::math_policy;
  using c = typename math_policy::constants;
  using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, 3>;

  auto const delta = aab_.hi - aab_.lo;
  rvec step;

  // TODO: introduce a "fit" parameter that determines if the particles are
  // aligned to the sides of the box, otherwise float them in regualr distances
  // in the center of the box (avoids problems with sampling fluid that
  // "explodes")

  integral_grid<N_> grid;
  for (size_t n = 0; n < N_; ++n) {
    grid.extents[n] =
        static_cast<size_t>(std::round(delta[n] / p_.maximum_sample_distance));
    step[n] = delta[n] / grid.extents[n];
  }

  rvec g_vec;
  for (auto const &g_arr : grid) {
    g_vec = c::template from_array<nd_dtype::real>(g_arr);
    *(it_++) = aab_.lo + (g_vec.array() * step.array()).matrix();
  }
}

} // namespace prtcl::rt
