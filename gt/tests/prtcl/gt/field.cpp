#include <catch.hpp>

#include <prtcl/gt/field.hpp>
#include <prtcl/gt/field_literals.hpp>

#include <type_traits>

TEST_CASE("prtcl/gt/field", "[prtcl]") {
  namespace gt = ::prtcl::gt;

  using fk = gt::field_kind;
  using ft = gt::field_type;

  REQUIRE(std::is_same<
          gt::field<fk::global, ft::real, ::prtcl::shape<>>,
          gt::scalar_field<fk::global, ft::real>>::value);

  REQUIRE(std::is_same<
          gt::field<fk::global, ft::real, ::prtcl::shape<0>>,
          gt::vector_field<fk::global, ft::real>>::value);

  REQUIRE(std::is_same<
          gt::field<fk::global, ft::real, ::prtcl::shape<0, 0>>,
          gt::matrix_field<fk::global, ft::real>>::value);

  SECTION("field literals are working correctly") {
    using namespace gt::field_literals;

    auto abc = "abc"_grsf;

    REQUIRE(fk::global == abc.kind());
    REQUIRE(ft::real == abc.type());
    REQUIRE(0 == abc.shape().rank());
    REQUIRE("abc" == abc.name());
  }
}
