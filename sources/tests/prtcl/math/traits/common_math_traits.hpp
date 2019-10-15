#pragma once

#include <catch.hpp>

#include <cmath>

template <typename MathTraits> void common_math_traits_tests() {
  using traits_type = MathTraits;

  typename traits_type::vector_type x{1.f, 2.f, 3.f}, y{4.f, 5.f, 6.f};

  REQUIRE(3 == traits_type::vector_extent);
  REQUIRE((1.f * 4.f + 2.f * 5.f + 3.f * 6.f) == traits_type::dot(x, y));
  REQUIRE(traits_type::dot(x, x) == traits_type::norm_squared(x));
  REQUIRE(std::sqrt(traits_type::norm_squared(x)) == traits_type::norm(x));
  REQUIRE(x / traits_type::norm(x) == traits_type::normalized(x));
}
