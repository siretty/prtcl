#include <catch.hpp>

#include <prtcl/expr/eq.hpp>

TEST_CASE("prtcl/expr/eq", "[prtcl][expr][eq]") {
  using namespace prtcl::expr_literals;

  prtcl::tag::group::active i;
  prtcl::tag::group::passive j;

  REQUIRE(prtcl::expr::_geq("a"_gs = 1 * "b"_gs));
  REQUIRE(prtcl::expr::_geq("a"_gs += 1 * "b"_gs));
  REQUIRE(prtcl::expr::_geq("a"_gs -= 1 * "b"_gs));

  REQUIRE(prtcl::expr::_ueq("a"_us[i] = 1 * "b"_gs));
  REQUIRE(prtcl::expr::_ueq("a"_us[i] += 1 * "b"_gs));
  REQUIRE(prtcl::expr::_ueq("a"_us[i] -= 1 * "b"_gs));

  REQUIRE(prtcl::expr::_veq("a"_vs[i] = 1 * "b"_gs));
  REQUIRE(prtcl::expr::_veq("a"_vs[i] += 1 * "b"_gs));
  REQUIRE(prtcl::expr::_veq("a"_vs[i] -= 1 * "b"_gs));

  REQUIRE(!prtcl::expr::_veq("a"_vs[j] -= 1 * "b"_gs));
}
