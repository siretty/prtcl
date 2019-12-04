#include <catch.hpp>

#include <prtcl/expr/ufield.hpp>

TEST_CASE("prtcl/expr/ufield", "[prtcl][expr][ufield]") {
  using namespace prtcl::expr::literals;

  using prtcl::tag::kind::uniform;
  using prtcl::tag::type::matrix;
  using prtcl::tag::type::scalar;
  using prtcl::tag::type::vector;

  auto us = "us"_us;
  REQUIRE(std::is_same<decltype(us)::kind_tag, uniform>::value);
  REQUIRE(std::is_same<decltype(us)::type_tag, scalar>::value);
  REQUIRE(us.value == "us");

  auto uv = "uv"_uv;
  REQUIRE(std::is_same<decltype(uv)::kind_tag, uniform>::value);
  REQUIRE(std::is_same<decltype(uv)::type_tag, vector>::value);
  REQUIRE(uv.value == "uv");

  auto um = "um"_um;
  REQUIRE(std::is_same<decltype(um)::kind_tag, uniform>::value);
  REQUIRE(std::is_same<decltype(um)::type_tag, matrix>::value);
  REQUIRE(um.value == "um");
}
