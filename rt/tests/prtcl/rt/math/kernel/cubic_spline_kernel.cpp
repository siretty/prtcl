#include <catch2/catch.hpp>

#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/common.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>
#include <prtcl/rt/math/kernel/cubic_spline_kernel.hpp>
#include <prtcl/rt/math/kernel_math_policy_mixin.hpp>
#include <prtcl/rt/math/mixed_math_policy.hpp>

#include <algorithm>
#include <random>
#include <utility>
#include <vector>

#include <boost/math/quadrature/naive_monte_carlo.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>

TEST_CASE("prtcl/rt/math/kernel/cubic_spline_kernel", "[prtcl][rt]") {
  using type_policy = prtcl::rt::fib_type_policy;
  using math_policy = typename prtcl::rt::mixed_math_policy<
      prtcl::rt::eigen_math_policy,
      prtcl::rt::kernel_math_policy_mixin<prtcl::rt::cubic_spline_kernel>>::
      template policy<type_policy>;
  using kernel_type = prtcl::rt::cubic_spline_kernel<math_policy>;

  using real = typename type_policy::real;

  using o = math_policy::operations;
  using c = math_policy::constants;

  using prtcl::core::nd_dtype;

  auto const h = static_cast<real>(1);

  CHECK("Cubic Spline" == kernel_type::get_name());

  CHECK(2 == o::kernel_support_radius(h));

  auto eval_at_zeros_d1 =
      o::kernel_h(c::template zeros<nd_dtype::real, 1>(), h);
  auto eval_at_zeros_d2 =
      o::kernel_h(c::template zeros<nd_dtype::real, 2>(), h);
  auto eval_at_zeros_d3 =
      o::kernel_h(c::template zeros<nd_dtype::real, 3>(), h);

  CHECK(eval_at_zeros_d1 > eval_at_zeros_d2);
  CHECK(eval_at_zeros_d2 > eval_at_zeros_d3);

  auto eval_at_pones_d1 = o::kernel_h(c::template ones<nd_dtype::real, 1>(), h);
  auto eval_at_nones_d1 =
      o::kernel_h(-c::template ones<nd_dtype::real, 1>(), h);

  CHECK(eval_at_pones_d1 < eval_at_zeros_d1);
  CHECK(eval_at_pones_d1 == eval_at_nones_d1);

  auto eval_at_pones_d2 = o::kernel_h(c::template ones<nd_dtype::real, 2>(), h);
  auto eval_at_nones_d2 =
      o::kernel_h(-c::template ones<nd_dtype::real, 2>(), h);

  CHECK(eval_at_pones_d2 < eval_at_zeros_d2);
  CHECK(eval_at_pones_d2 == eval_at_nones_d2);

  auto eval_at_pones_d3 = o::kernel_h(c::template ones<nd_dtype::real, 3>(), h);
  auto eval_at_nones_d3 =
      o::kernel_h(-c::template ones<nd_dtype::real, 3>(), h);

  CHECK(eval_at_pones_d3 < eval_at_zeros_d3);
  CHECK(eval_at_pones_d3 == eval_at_nones_d3);

  auto grad_at_zeros_d1 =
      o::kernel_gradient_h(c::template zeros<nd_dtype::real, 1>(), h);
  auto grad_at_zeros_d2 =
      o::kernel_gradient_h(c::template zeros<nd_dtype::real, 2>(), h);
  auto grad_at_zeros_d3 =
      o::kernel_gradient_h(c::template zeros<nd_dtype::real, 3>(), h);

  CHECK(0 == o::norm(grad_at_zeros_d1));
  CHECK(0 == o::norm(grad_at_zeros_d2));
  CHECK(0 == o::norm(grad_at_zeros_d3));

  auto grad_at_pones_d1 =
      o::kernel_gradient_h(c::template ones<nd_dtype::real, 1>(), h);

  CHECK(grad_at_pones_d1[0] < 0);
  CHECK(o::cmin(grad_at_pones_d1) == o::cmax(grad_at_pones_d1));

  auto grad_at_pones_d2 =
      o::kernel_gradient_h(c::template ones<nd_dtype::real, 2>(), h);

  CHECK(grad_at_pones_d2[0] < 0);
  CHECK(o::cmin(grad_at_pones_d2) == o::cmax(grad_at_pones_d2));

  auto grad_at_pones_d3 =
      o::kernel_gradient_h(c::template ones<nd_dtype::real, 3>(), h);

  CHECK(grad_at_pones_d3[0] < 0);
  CHECK(o::cmin(grad_at_pones_d3) == o::cmax(grad_at_pones_d3));

  SECTION("integrate kernel") {
    auto const error_goal = static_cast<real>(.0001);

    SECTION("1D") {
      auto eval = [h](auto const &x) {
        return o::kernel_h(math_policy::nd_dtype_t<nd_dtype::real, 1>{x[0]}, h);
      };
      std::vector<std::pair<real, real>> bounds = {
          {0, o::kernel_support_radius(h)},
      };

      boost::math::quadrature::naive_monte_carlo<real, decltype(eval)> mc{
          eval, bounds, error_goal};
      real const value = mc.integrate().get();

      CHECK(Approx(.5).margin(10 * error_goal) == value);
    }

    SECTION("2D") {
      auto eval = [h](auto const &x) {
        return o::kernel_h(
            math_policy::nd_dtype_t<nd_dtype::real, 2>{x[0], x[1]}, h);
      };
      std::vector<std::pair<real, real>> bounds = {
          {0, o::kernel_support_radius(h)},
          {0, o::kernel_support_radius(h)},
      };

      boost::math::quadrature::naive_monte_carlo<real, decltype(eval)> mc{
          eval, bounds, error_goal};
      real const value = mc.integrate().get();

      CHECK(Approx(.25).margin(10 * error_goal) == value);
    }

    SECTION("3D") {
      auto eval = [h](auto const &x) {
        return o::kernel_h(
            math_policy::nd_dtype_t<nd_dtype::real, 3>{x[0], x[1], x[2]}, h);
      };
      std::vector<std::pair<real, real>> bounds = {
          {0, o::kernel_support_radius(h)},
          {0, o::kernel_support_radius(h)},
          {0, o::kernel_support_radius(h)},
      };

      boost::math::quadrature::naive_monte_carlo<real, decltype(eval)> mc{
          eval, bounds, error_goal};
      real const value = mc.integrate().get();

      CHECK(Approx(.125).margin(10 * error_goal) == value);
    }
  }

  SECTION("kernel radial symmetry") {
    auto const radius = o::kernel_support_radius(h);

    std::mt19937 gen{0};
    std::uniform_real_distribution<real> dis{-radius, radius};

    auto identity = [](auto &&arg_) {
      return std::forward<decltype(arg_)>(arg_);
    };

    SECTION("1D") {
      using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, 1>;

      std::vector<rvec> points(1000);
      for (size_t i = 0; i < points.size(); ++i) {
        points[i][0] = dis(gen);
      }

      auto results =
          boost::irange<size_t>(0, points.size()) |
          boost::adaptors::transformed([&points, h](auto i_) -> bool {
            return 0 ==
                   o::kernel_h(points[i_], h) - o::kernel_h(-points[i_], h);
          });

      CHECK(std::all_of(results.begin(), results.end(), identity));
    }

    SECTION("2D") {
      using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, 2>;

      std::vector<rvec> points(1000);
      for (size_t i = 0; i < points.size(); ++i) {
        points[i][0] = dis(gen);
        points[i][1] = dis(gen);
      }

      auto results =
          boost::irange<size_t>(0, points.size()) |
          boost::adaptors::transformed([&points, h](auto i_) -> bool {
            return 0 ==
                   o::kernel_h(points[i_], h) - o::kernel_h(-points[i_], h);
          });

      CHECK(std::all_of(results.begin(), results.end(), identity));
    }

    SECTION("3D") {
      using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, 3>;

      std::vector<rvec> points(1000);
      for (size_t i = 0; i < points.size(); ++i) {
        points[i][0] = dis(gen);
        points[i][1] = dis(gen);
        points[i][2] = dis(gen);
      }

      auto results =
          boost::irange<size_t>(0, points.size()) |
          boost::adaptors::transformed([&points, h](auto i_) -> bool {
            return 0 ==
                   o::kernel_h(points[i_], h) - o::kernel_h(-points[i_], h);
          });

      CHECK(std::all_of(results.begin(), results.end(), identity));
    }
  }

  SECTION("kernel gradient point-symmetry") {
    auto const radius = o::kernel_support_radius(h);

    std::mt19937 gen{0};
    std::uniform_real_distribution<real> dis{-radius, radius};

    auto identity = [](auto &&arg_) {
      return std::forward<decltype(arg_)>(arg_);
    };

    SECTION("1D") {
      using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, 1>;

      std::vector<rvec> points(1000);
      for (size_t i = 0; i < points.size(); ++i) {
        points[i][0] = dis(gen);
      }

      auto results =
          boost::irange<size_t>(0, points.size()) |
          boost::adaptors::transformed([&points, h](auto i_) -> bool {
            return 0 == o::norm(
                            o::kernel_gradient_h(points[i_], h) +
                            o::kernel_gradient_h(-points[i_], h));
          });

      CHECK(std::all_of(results.begin(), results.end(), identity));
    }

    SECTION("2D") {
      using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, 2>;

      std::vector<rvec> points(1000);
      for (size_t i = 0; i < points.size(); ++i) {
        points[i][0] = dis(gen);
        points[i][1] = dis(gen);
      }

      auto results =
          boost::irange<size_t>(0, points.size()) |
          boost::adaptors::transformed([&points, h](auto i_) -> bool {
            return 0 == o::norm(
                            o::kernel_gradient_h(points[i_], h) +
                            o::kernel_gradient_h(-points[i_], h));
          });

      CHECK(std::all_of(results.begin(), results.end(), identity));
    }

    SECTION("3D") {
      using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, 3>;

      std::vector<rvec> points(1000);
      for (size_t i = 0; i < points.size(); ++i) {
        points[i][0] = dis(gen);
        points[i][1] = dis(gen);
        points[i][2] = dis(gen);
      }

      auto results =
          boost::irange<size_t>(0, points.size()) |
          boost::adaptors::transformed([&points, h](auto i_) -> bool {
            return 0 == o::norm(
                            o::kernel_gradient_h(points[i_], h) +
                            o::kernel_gradient_h(-points[i_], h));
          });

      CHECK(std::all_of(results.begin(), results.end(), identity));
    }
  }
}
