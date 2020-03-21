#include <catch2/catch.hpp>

#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>

#include <limits>
#include <type_traits>

TEST_CASE("prtcl/rt/math/eigen_math_policy.hpp", "[prtcl][rt]") {
  using type_policy = prtcl::rt::fib_type_policy;
  using math_policy = prtcl::rt::eigen_math_policy<type_policy>;

  using prtcl::rt::nd_boolean_t;
  using prtcl::rt::nd_integer_t;
  using prtcl::rt::nd_real_t;

  SECTION("math policy types") {
    CHECK(std::is_same<nd_real_t<math_policy>, float>::value);
    CHECK(std::is_same<nd_integer_t<math_policy>, int>::value);
    CHECK(std::is_same<nd_boolean_t<math_policy>, bool>::value);
  }

  using prtcl::core::dtype;

  using o = typename math_policy::operations;

  SECTION("scalars") {
    CHECK(0.f == o::zeros<dtype::real>());
    CHECK(0 == o::zeros<dtype::integer>());
    CHECK(false == o::zeros<dtype::boolean>());

    CHECK(1.f == o::ones<dtype::real>());
    CHECK(1 == o::ones<dtype::integer>());
    CHECK(true == o::ones<dtype::boolean>());
  }

  SECTION("three-dimensional real linear algebra") {
    using rvec3 = nd_real_t<math_policy, 3>;
    using rmat3 = nd_real_t<math_policy, 3, 3>;

    CHECK(rvec3{0.f, 0.f, 0.f} == o::zeros<dtype::real, 3>());
    CHECK(rvec3{1.f, 1.f, 1.f} == o::ones<dtype::real, 3>());

    CHECK(
        o::zeros<dtype::real, 3>() ==
        o::narray<dtype::real, 3>({{0.f, 0.f, 0.f}}));

    CHECK(o::most_positive<dtype::real>() == std::numeric_limits<float>::max());
    CHECK(
        o::most_negative<dtype::real>() ==
        std::numeric_limits<float>::lowest());

    rmat3 I;
    I << 1, 0, 0, //
        0, 1, 0,  //
        0, 0, 1;
    CHECK(I == o::identity<dtype::real, 3, 3>());

    CHECK(
        o::positive_infinity<dtype::real>() ==
        std::numeric_limits<float>::infinity());

    CHECK(
        o::negative_infinity<dtype::real>() ==
        -std::numeric_limits<float>::infinity());
  }

  SECTION("three-dimensional integer vectors") {
    using ivec3 = nd_integer_t<math_policy, 3>;

    CHECK(ivec3{0, 0, 0} == o::zeros<dtype::integer, 3>());
    CHECK(ivec3{1, 1, 1} == o::ones<dtype::integer, 3>());

    CHECK(
        o::most_positive<dtype::integer>() == std::numeric_limits<int>::max());
    CHECK(
        o::most_negative<dtype::integer>() ==
        std::numeric_limits<int>::lowest());
  }

  SECTION("three-dimensional boolean vectors") {
    using bvec3 = nd_boolean_t<math_policy, 3>;

    CHECK(bvec3{false, false, false} == o::zeros<dtype::boolean, 3>());
    CHECK(bvec3{true, true, true} == o::ones<dtype::boolean, 3>());
  }

  // using o = typename math_policy::operations;
}
