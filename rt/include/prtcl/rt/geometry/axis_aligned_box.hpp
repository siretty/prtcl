#pragma once

#include <prtcl/rt/common.hpp>

#include <cstddef>

namespace prtcl::rt {

template <typename MathPolicy_, size_t N_> struct axis_aligned_box {
  using math_policy = MathPolicy_;
  using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, N_>;

  static constexpr size_t dimensionality = N_;

private:
  using c = typename math_policy::constants;
  using o = typename math_policy::operations;

public:
  axis_aligned_box(
      rvec radius_ = c::template ones<nd_dtype::real, N_>(),
      rvec center_ = c::template zeros<nd_dtype::real, N_>())
      : _radius{radius_}, _center{center_} {}

public:
  template <typename Point> auto distance(Point &&point_) const {
    auto const zeros = c::template zeros<nd_dtype::real, N_>();
    auto const t = o::cabs(point_ - _center) - _radius;
    auto const max_t = o::maximum_component(t);
    return (max_t >= 0) ? o::norm(o::cmax(t, zeros)) : max_t;
  }

public:
  decltype(auto) lo() const { return _center - _radius; }

  decltype(auto) hi() const { return _center + _radius; }

public:
  auto const &radius() const { return _radius; }

  auto const &center() const { return _center; }

private:
  rvec _radius = c::template ones<nd_dtype::real, N_>();
  rvec _center = c::template zeros<nd_dtype::real, N_>();
};

} // namespace prtcl::rt
