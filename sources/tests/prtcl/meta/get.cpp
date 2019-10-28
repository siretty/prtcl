#include <catch.hpp>

#include <prtcl/meta/get.hpp>

#include <array>
#include <utility>

TEST_CASE("prtcl/meta/get", "[prtcl][meta][get]") {
  std::array<int, 3> const a{0, 1, 2};
  REQUIRE(0 == prtcl::meta::get<0>(a));
  REQUIRE(1 == prtcl::meta::get<1>(a));
  REQUIRE(2 == prtcl::meta::get<2>(a));

  auto const s = std::integer_sequence<int, 0, 1, 2>{};
  REQUIRE(0 == prtcl::meta::get<0>(s));
  REQUIRE(1 == prtcl::meta::get<1>(s));
  REQUIRE(2 == prtcl::meta::get<2>(s));
}
