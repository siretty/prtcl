#include <catch2/catch.hpp>

#include <prtcl/gt/field.hpp>
#include <prtcl/gt/field_literals.hpp>

#include <type_traits>

#include <boost/lexical_cast.hpp>

TEST_CASE("prtcl/gt/field", "[prtcl]") {
  namespace gt = ::prtcl::gt;

  using fk = gt::field_kind;
  using ft = gt::field_type;

  SECTION("custom constructor") {
    gt::field abc{fk::varying, ft::real, {1, 2, 3}, "abc"};
    REQUIRE(fk::varying == abc.kind());
    REQUIRE(ft::real == abc.type());
    REQUIRE(3 == abc.rank());
    REQUIRE(std::array{1, 2, 3} == abc.shape());
    REQUIRE("abc" == abc.name());
  }

  SECTION("field literals are working correctly") {
    using namespace gt::field_literals;
    using boost::lexical_cast;

    SECTION("global real scalar") {
      auto abc = "abc"_grs;

      REQUIRE(fk::global == abc.kind());
      REQUIRE(ft::real == abc.type());
      REQUIRE(0 == abc.rank());
      REQUIRE("abc" == abc.name());
      REQUIRE(
          "field{global, real, {}, \"abc\"}" == lexical_cast<std::string>(abc));
    }

    SECTION("global real vector") {
      auto abc = "abc"_grv;

      REQUIRE(fk::global == abc.kind());
      REQUIRE(ft::real == abc.type());
      REQUIRE(1 == abc.rank());
      REQUIRE(0 == abc.shape()[0]);
      REQUIRE("abc" == abc.name());
    }

    SECTION("global real matrix") {
      auto abc = "abc"_grm;

      REQUIRE(fk::global == abc.kind());
      REQUIRE(ft::real == abc.type());
      REQUIRE(2 == abc.rank());
      REQUIRE(0 == abc.shape()[0]);
      REQUIRE(0 == abc.shape()[1]);
      REQUIRE("abc" == abc.name());
    }
  }
}
