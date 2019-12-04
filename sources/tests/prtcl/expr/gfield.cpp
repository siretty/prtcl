#include <catch.hpp>

#include <prtcl/expr/gfield.hpp>

TEST_CASE("prtcl/expr/gfield", "[prtcl][expr][gfield]") {
  using namespace prtcl::expr::literals;

  using prtcl::tag::kind::global;
  using prtcl::tag::type::matrix;
  using prtcl::tag::type::scalar;
  using prtcl::tag::type::vector;

  auto gs = "gs"_gs;
  REQUIRE(std::is_same<decltype(gs)::kind_tag, global>::value);
  REQUIRE(std::is_same<decltype(gs)::type_tag, scalar>::value);
  REQUIRE(gs.value == "gs");

  auto gv = "gv"_gv;
  REQUIRE(std::is_same<decltype(gv)::kind_tag, global>::value);
  REQUIRE(std::is_same<decltype(gv)::type_tag, vector>::value);
  REQUIRE(gv.value == "gv");

  auto gm = "gm"_gm;
  REQUIRE(std::is_same<decltype(gm)::kind_tag, global>::value);
  REQUIRE(std::is_same<decltype(gm)::type_tag, matrix>::value);
  REQUIRE(gm.value == "gm");
}
