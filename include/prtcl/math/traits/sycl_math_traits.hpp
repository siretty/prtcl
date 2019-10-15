#pragma once

#include <utility>

#include <cstddef>

#include "../../libs/sycl.hpp"

namespace prtcl {

template <typename T, size_t N> struct sycl_math_traits {
  using scalar_type = T;
  using vector_type = sycl::vec<T, static_cast<int>(N)>;

  constexpr static size_t vector_extent = N;

  template <typename LHS, typename RHS> static auto dot(LHS &&lhs, RHS &&rhs) {
    return sycl::dot(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
  }

  template <typename Arg> static auto norm(Arg &&arg) {
    return sycl::length(std::forward<Arg>(arg));
  }

  template <typename Arg> static auto norm_squared(Arg &&arg) {
    return dot(std::forward<Arg>(arg), std::forward<Arg>(arg));
  }

  template <typename Arg> static auto normalized(Arg &&arg) {
    return sycl::normalize(std::forward<Arg>(arg));
  }

  static auto make_zero_vector() {
    return vector_type{static_cast<scalar_type>(0)};
  }

  static scalar_type epsilon() {
    return std::numeric_limits<scalar_type>::epsilon();
  }
};

} // namespace prtcl
