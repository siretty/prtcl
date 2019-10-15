#pragma once

#include <catch.hpp>

#include <algorithm>
#include <vector>

#include <cmath>

#include <boost/math/quadrature/naive_monte_carlo.hpp>

template <typename Kernel> void common_math_kernel_tests() {
  using kernel_type = Kernel;
  using math_traits = typename kernel_type::math_traits;
  using scalar_type = typename math_traits::scalar_type;
  constexpr size_t vector_extent = math_traits::vector_extent;

  auto h = static_cast<scalar_type>(0.025);

  kernel_type kernel;

  scalar_type H = kernel.get_support_radius(h);

  { // check scalar kernel value
    REQUIRE(0 < kernel.evalr(0, h));
    REQUIRE(0 < kernel.evalr(H / 2, h));
    REQUIRE(0 < kernel.evalr(-H / 2, h));

    // support radius edge and beyond
    REQUIRE(0 == kernel.evalr(H, h));
    REQUIRE(0 == kernel.evalr(2 * H, h));
    REQUIRE(0 == kernel.evalr(-H, h));
    REQUIRE(0 == kernel.evalr(-2 * H, h));
  }

  { // check scalar kernel derivative
    REQUIRE(0 == kernel.evaldr(0, h));
    REQUIRE(0 < kernel.evaldr(-h, h));
    REQUIRE(0 > kernel.evaldr(h, h));

    // support radius edge and beyond
    REQUIRE(0 == kernel.evaldr(H, h));
    REQUIRE(0 == kernel.evaldr(2 * H, h));
    REQUIRE(0 == kernel.evaldr(-H, h));
    REQUIRE(0 == kernel.evaldr(-2 * H, h));
  }

  { // check integral
    auto W = [kernel, h](auto const &x) {
      return kernel.evalr(std::sqrt(std::inner_product(
                              x.begin(), x.end(), x.begin(), scalar_type{0})),
                          h);
    };

    std::vector<std::pair<scalar_type, scalar_type>> bounds(vector_extent);
    for (auto &b : bounds) {
      b.first = -H;
      b.second = H;
    }

    auto error_goal = static_cast<scalar_type>(0.001);

    boost::math::quadrature::naive_monte_carlo<scalar_type, decltype(W)> mc{
        W, bounds, error_goal};
    auto task = mc.integrate();

    REQUIRE(Approx(1).epsilon(10 * error_goal) == task.get());
  }
}
