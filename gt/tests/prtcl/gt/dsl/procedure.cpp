#include <catch.hpp>

#include <prtcl/gt/dsl.hpp>

TEST_CASE("prtcl/gt/dsl/procedure", "[prtcl]") {
  using namespace ::prtcl::gt::dsl::language;
  using namespace ::prtcl::gt::dsl::monaghan_indices;
  using namespace ::prtcl::gt::dsl::kernel_shorthand;

  auto const x = vr_field("position", {0});

  procedure(
      "test",                                   //
      foreach_particle(                         //
          if_group_type(                        //
              "some_type",                      //
              x[a] = zeros({0}),                //
              x[a] = dot(x[a], x[a]),           //
              x[a] = norm(ones({0})) * dW(x[a]) //
              )                                 //
          )                                     //
  );
}
