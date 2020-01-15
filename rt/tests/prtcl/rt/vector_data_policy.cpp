#include <catch.hpp>

#include <prtcl/rt/basic_model.hpp>
#include <prtcl/rt/basic_model_policy.hpp>
#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>
#include <prtcl/rt/vector_data_policy.hpp>

#include <limits>
#include <type_traits>

TEST_CASE("prtcl/rt/math/vector_data_policy", "[prtcl][rt]") {
  using model_policy = prtcl::rt::basic_model_policy<
      prtcl::rt::fib_type_policy, prtcl::rt::eigen_math_policy,
      prtcl::rt::vector_data_policy, 3>;

  using prtcl::core::nd_dtype;

  prtcl::rt::basic_model<model_policy> model;
  CHECK(0 == model.groups().size());

  SECTION("adding globals") {
    SECTION("real scalar") {
      auto added = model.add_global<nd_dtype::real>("greal");
      CHECK(added == model.get_global<nd_dtype::real>("greal"));
      CHECK(nd_dtype::real == added.dtype());
      CHECK(0 == added.shape().size());
    }

    SECTION("real three-dim. vector") {
      auto added = model.add_global<nd_dtype::real, 3>("grvec3");
      CHECK(added == model.get_global<nd_dtype::real, 3>("grvec3"));
      CHECK(nd_dtype::real == added.dtype());
      CHECK(1 == added.shape().size());
      CHECK(3 == added.shape()[0]);
    }

    SECTION("real three-dim. matrix") {
      auto added = model.add_global<nd_dtype::real, 3, 3>("grmat3");
      CHECK(added == model.get_global<nd_dtype::real, 3, 3>("grmat3"));
      CHECK(nd_dtype::real == added.dtype());
      CHECK(2 == added.shape().size());
      CHECK(3 == added.shape()[0]);
      CHECK(3 == added.shape()[1]);
    }
  }

  SECTION("adding group") {
    auto &group = model.add_group("group", "fluid");
    CHECK(0 == group.size());
    CHECK("group" == group.get_name());
    CHECK("fluid" == group.get_type());
    CHECK(1 == model.groups().size());
    CHECK(group.get_name() == model.groups()[0].get_name());

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
