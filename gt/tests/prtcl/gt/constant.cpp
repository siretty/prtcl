#include <catch.hpp>

#include <prtcl/gt/constant.hpp>
#include <prtcl/gt/constant_literals.hpp>

#include <type_traits>

#include <boost/lexical_cast.hpp>

TEST_CASE("prtcl/gt/constant", "[prtcl]") {
  namespace gt = ::prtcl::gt;

  using ft = gt::field_type;

  SECTION("custom constructor") {
    gt::constant zeros{ft::real, {1, 2, 3}, "zeros"};
    CHECK(ft::real == zeros.type());
    CHECK(3 == zeros.rank());
    CHECK(std::array{1, 2, 3} == zeros.shape());
    CHECK("zeros" == zeros.content());
  }

  SECTION("constant literals are working correctly") {
    using namespace gt::constant_literals;
    using ::boost::lexical_cast;

    SECTION("real scalar") {
      CHECK(
          "constant{real, {}, \"abc\"}" ==
          lexical_cast<std::string>("abc"_crs));
    }
  }
}
