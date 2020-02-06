#include <catch2/catch.hpp>

#include <prtcl/gt/field_type.hpp>

TEST_CASE("prtcl/gt/field_type", "[prtcl]") {
  using ft = ::prtcl::gt::field_type;

  REQUIRE("real" == enumerator_name(ft::real));
  // TODO:
  // REQUIRE("index" == enumerator_name(ft::index));
  // TODO:
  // REQUIRE("boolean" == enumerator_name(ft::boolean));

  REQUIRE_THROWS_AS(
      enumerator_name(static_cast<ft>(-1)),
      ::prtcl::invalid_enumerator_error<ft>);
}
