#pragma once

#include <prtcl/rt/common.hpp>

#include <array>

#include <cstddef>

namespace prtcl::rt {

template <typename ModelPolicy_, size_t N_> struct axis_aligned_box {
  using model_policy = ModelPolicy_;
  using math_policy = typename model_policy::math_policy;
  using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, N_>;

  rvec lo, hi;
};

} // namespace prtcl::rt
