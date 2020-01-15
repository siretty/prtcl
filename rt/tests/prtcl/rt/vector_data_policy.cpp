#include <catch.hpp>

#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>
#include <prtcl/rt/vector_data_policy.hpp>

#include <limits>
#include <type_traits>

TEST_CASE("prtcl/rt/math/vector_data_policy", "[prtcl][rt]") {
  using type_policy = prtcl::rt::fib_type_policy;
  using math_policy = prtcl::rt::eigen_math_policy<type_policy>;
  using data_policy = prtcl::rt::vector_data_policy<math_policy>;

  using prtcl::core::nd_dtype;

  typename data_policy::scheme_type scheme;
  CHECK(0 == scheme.groups().size());

  SECTION("adding globals") {
    SECTION("real scalar") {
      auto added = scheme.add_global<nd_dtype::real>("greal");
      CHECK(added == scheme.get_global<nd_dtype::real>("greal"));
      CHECK(nd_dtype::real == added.dtype());
      CHECK(0 == added.shape().size());
    }

    SECTION("real three-dim. vector") {
      auto added = scheme.add_global<nd_dtype::real, 3>("grvec3");
      CHECK(added == scheme.get_global<nd_dtype::real, 3>("grvec3"));
      CHECK(nd_dtype::real == added.dtype());
      CHECK(1 == added.shape().size());
      CHECK(3 == added.shape()[0]);
    }

    SECTION("real three-dim. matrix") {
      auto added = scheme.add_global<nd_dtype::real, 3, 3>("grmat3");
      CHECK(added == scheme.get_global<nd_dtype::real, 3, 3>("grmat3"));
      CHECK(nd_dtype::real == added.dtype());
      CHECK(2 == added.shape().size());
      CHECK(3 == added.shape()[0]);
      CHECK(3 == added.shape()[1]);
    }
  }

  SECTION("adding group") {
    auto &group = scheme.add_group("group", "fluid");
    CHECK(0 == group.size());
    CHECK("group" == group.get_name());
    CHECK("fluid" == group.get_type());
    CHECK(1 == scheme.groups().size());
    CHECK(group.get_name() == scheme.groups()[0].get_name());

    SECTION("adding uniforms") {
      SECTION("real scalar") {
        auto added = group.add_uniform<nd_dtype::real>("ureal");
        CHECK(added == group.get_uniform<nd_dtype::real>("ureal"));
        CHECK(1 == added.size());
        CHECK(nd_dtype::real == added.dtype());
        CHECK(0 == added.shape().size());
      }

      SECTION("real three-dim. vector") {
        auto added = group.add_uniform<nd_dtype::real, 3>("urvec3");
        CHECK(added == group.get_uniform<nd_dtype::real, 3>("urvec3"));
        CHECK(1 == added.size());
        CHECK(nd_dtype::real == added.dtype());
        CHECK(1 == added.shape().size());
        CHECK(3 == added.shape()[0]);
      }

      SECTION("real three-dim. matrix") {
        auto added = group.add_uniform<nd_dtype::real, 3, 3>("urmat3");
        CHECK(added == group.get_uniform<nd_dtype::real, 3, 3>("urmat3"));
        CHECK(1 == added.size());
        CHECK(nd_dtype::real == added.dtype());
        CHECK(2 == added.shape().size());
        CHECK(3 == added.shape()[0]);
        CHECK(3 == added.shape()[1]);
      }
    }

    SECTION("adding varyings") {
      SECTION("real scalar") {
        auto added = group.add_varying<nd_dtype::real>("vreal");
        CHECK(added == group.get_varying<nd_dtype::real>("vreal"));
        CHECK(0 == added.size());
        CHECK(nd_dtype::real == added.dtype());
        CHECK(0 == added.shape().size());
      }

      SECTION("real three-dim. vector") {
        auto added = group.add_varying<nd_dtype::real, 3>("urvec3");
        CHECK(added == group.get_varying<nd_dtype::real, 3>("urvec3"));
        CHECK(0 == added.size());
        CHECK(nd_dtype::real == added.dtype());
        CHECK(1 == added.shape().size());
        CHECK(3 == added.shape()[0]);
      }

      SECTION("real three-dim. matrix") {
        auto added = group.add_varying<nd_dtype::real, 3, 3>("urmat3");
        CHECK(added == group.get_varying<nd_dtype::real, 3, 3>("urmat3"));
        CHECK(0 == added.size());
        CHECK(nd_dtype::real == added.dtype());
        CHECK(2 == added.shape().size());
        CHECK(3 == added.shape()[0]);
        CHECK(3 == added.shape()[1]);
      }
    }

    SECTION("check size of varyings") {
      CHECK(0 == group.size());

      CHECK(0 == group.add_varying<nd_dtype::real>("vreal").size());
      CHECK(0 == group.add_varying<nd_dtype::real, 3>("urvec3").size());
      CHECK(0 == group.add_varying<nd_dtype::real, 3, 3>("urmat3").size());

      group.resize(1234);
      CHECK(1234 == group.size());

      CHECK(1234 == group.get_varying<nd_dtype::real>("vreal").size());
      CHECK(1234 == group.get_varying<nd_dtype::real, 3>("urvec3").size());
      CHECK(1234 == group.get_varying<nd_dtype::real, 3, 3>("urmat3").size());
    }
  }
}
