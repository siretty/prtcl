#include <catch.hpp>

#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>

#include <limits>
#include <type_traits>

TEST_CASE("prtcl/rt/math/eigen_math_policy.hpp", "[prtcl][rt]") {
  using type_policy = prtcl::rt::fib_type_policy;
  using math_policy = prtcl::rt::eigen_math_policy<type_policy>;

  SECTION("math policy types") {
    CHECK(std::is_same<typename math_policy::real, float>::value);
    CHECK(std::is_same<typename math_policy::integer, int>::value);
    CHECK(std::is_same<typename math_policy::boolean, bool>::value);

    CHECK(std::is_same<typename math_policy::nd_real_t<>, float>::value);
    CHECK(std::is_same<typename math_policy::nd_integer_t<>, int>::value);
    CHECK(std::is_same<typename math_policy::nd_boolean_t<>, bool>::value);
  }

  using prtcl::core::nd_dtype;

  using c = typename math_policy::constants;

  SECTION("scalars") {
    CHECK(0.f == c::zeros<nd_dtype::real>());
    CHECK(0 == c::zeros<nd_dtype::integer>());
    CHECK(false == c::zeros<nd_dtype::boolean>());

    CHECK(1.f == c::ones<nd_dtype::real>());
    CHECK(1 == c::ones<nd_dtype::integer>());
    CHECK(true == c::ones<nd_dtype::boolean>());
  }

  SECTION("three-dimensional real linear algebra") {
    using rvec3 = typename math_policy::nd_real_t<3>;
    using rmat3 = typename math_policy::nd_real_t<3, 3>;

    CHECK(rvec3{0.f, 0.f, 0.f} == c::zeros<nd_dtype::real, 3>());
    CHECK(rvec3{1.f, 1.f, 1.f} == c::ones<nd_dtype::real, 3>());

    CHECK(
        c::most_positive<nd_dtype::real>() ==
        std::numeric_limits<float>::max());
    CHECK(
        c::most_negative<nd_dtype::real>() ==
        std::numeric_limits<float>::lowest());

    rmat3 I;
    I << 1, 0, 0, //
        0, 1, 0,  //
        0, 0, 1;
    CHECK(I == c::identity<nd_dtype::real, 3, 3>());

    CHECK(
        c::positive_infinity<nd_dtype::real>() ==
        std::numeric_limits<float>::infinity());

    CHECK(
        c::negative_infinity<nd_dtype::real>() ==
        -std::numeric_limits<float>::infinity());
  }

  SECTION("three-dimensional integer vectors") {
    using ivec3 = typename math_policy::nd_integer_t<3>;

    CHECK(ivec3{0, 0, 0} == c::zeros<nd_dtype::integer, 3>());
    CHECK(ivec3{1, 1, 1} == c::ones<nd_dtype::integer, 3>());

    CHECK(
        c::most_positive<nd_dtype::integer>() ==
        std::numeric_limits<int>::max());
    CHECK(
        c::most_negative<nd_dtype::integer>() ==
        std::numeric_limits<int>::lowest());
  }

  SECTION("three-dimensional boolean vectors") {
    using bvec3 = typename math_policy::nd_boolean_t<3>;

    CHECK(bvec3{false, false, false} == c::zeros<nd_dtype::boolean, 3>());
    CHECK(bvec3{true, true, true} == c::ones<nd_dtype::boolean, 3>());
  }

  // using o = typename math_policy::operations;
}
