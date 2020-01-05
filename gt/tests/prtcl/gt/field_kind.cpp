#include <catch.hpp>

#include <prtcl/gt/field_kind.hpp>

TEST_CASE("prtcl/gt/field_kind", "[prtcl]") {
  using fk = ::prtcl::gt::field_kind;

  REQUIRE("global" == enumerator_name(fk::global));
  REQUIRE("uniform" == enumerator_name(fk::uniform));
  REQUIRE("varying" == enumerator_name(fk::varying));

  REQUIRE_THROWS_AS(
      enumerator_name(static_cast<fk>(-1)),
      ::prtcl::invalid_enumerator_error<fk>);
}
