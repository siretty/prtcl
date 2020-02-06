#include <catch2/catch.hpp>

#include <prtcl/gt/field_shape.hpp>

TEST_CASE("prtcl/gt/field_shape", "[prtcl]") {
  namespace gt = ::prtcl::gt;

  SECTION("static member completed is working") {
    REQUIRE(not gt::field_shape<0, 1, 2, 3>::completed());
    REQUIRE(gt::field_shape<1, 1, 2, 3>::completed());
  }

  SECTION("static member complete is working") {
    constexpr gt::field_extent extent = 2;

    using incomplete = gt::field_shape<0, 1, 0>;
    using completed = gt::field_shape<extent, 1, extent>;

    REQUIRE(completed{} == incomplete::complete<2>());
    REQUIRE(completed{} == gt::completed_field_shape_t<incomplete, 2>{});
  }

  SECTION("static member rank is working") {
    REQUIRE(0 == gt::field_shape<>::rank());
    REQUIRE(1 == gt::field_shape<0>::rank());
    REQUIRE(1 == gt::field_shape<1>::rank());
    REQUIRE(2 == gt::field_shape<0, 0>::rank());
    REQUIRE(2 == gt::field_shape<0, 1>::rank());
    REQUIRE(2 == gt::field_shape<1, 0>::rank());
    REQUIRE(2 == gt::field_shape<1, 1>::rank());
  }
}
