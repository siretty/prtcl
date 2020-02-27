#pragma once

#include <prtcl/rt/common.hpp>

#include <cmath>
#include <cstddef>

namespace prtcl::rt {

template <typename MathPolicy_, size_t N_> class sphere {
public:
  using math_policy = MathPolicy_;
  using type_policy = typename math_policy::type_policy;
  using real = typename type_policy::real;
  using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, N_>;

  static constexpr size_t dimensionality = N_;

private:
  using c = typename math_policy::constants;
  using o = typename math_policy::operations;

public:
  sphere(
      rvec center_ = c::template zeros<nd_dtype::real, N_>(), real radius_ = 1)
      : _center{center_}, _radius{radius_} {}

public:
  template <typename Point> auto distance(Point &&point_) const {
    return o::norm(std::forward<Point>(point_) - _center) - _radius;
  }

public:
  auto const &center() const { return _center; }

  auto radius() const { return _radius; }

private:
  rvec _center;
  real _radius;
};

} // namespace prtcl::rt
