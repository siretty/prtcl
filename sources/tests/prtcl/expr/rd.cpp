#include <catch.hpp>

#include <prtcl/expr/rd.hpp>

TEST_CASE("prtcl/expr/rd", "[prtcl][expr][rd]") {
  using namespace prtcl::expr_literals;

  prtcl::tag::group::active i;
  prtcl::tag::group::passive j;

  REQUIRE(!prtcl::expr::_grd("a"_gs = 1 * "b"_gs));
  REQUIRE(prtcl::expr::_grd("a"_gs += 1 * "b"_gs));
  REQUIRE(prtcl::expr::_grd("a"_gs -= 1 * "b"_gs));
  REQUIRE(prtcl::expr::_grd("a"_gs *= 1 * "b"_gs));
  REQUIRE(prtcl::expr::_grd("a"_gs /= 1 * "b"_gs));

  REQUIRE(!prtcl::expr::_urd("a"_us[i] = 1 * "b"_us[j]));
  REQUIRE(!prtcl::expr::_urd("a"_us[j] = 1 * "b"_us[i]));
  REQUIRE(!prtcl::expr::_urd("a"_us[j] += 1 * "b"_us[i]));
  REQUIRE(!prtcl::expr::_urd("a"_us[j] -= 1 * "b"_us[i]));
  REQUIRE(!prtcl::expr::_urd("a"_us[j] *= 1 * "b"_us[i]));
  REQUIRE(!prtcl::expr::_urd("a"_us[j] /= 1 * "b"_us[i]));

  REQUIRE(prtcl::expr::_urd("a"_us[i] += 1 * "b"_us[j]));
  REQUIRE(prtcl::expr::_urd("a"_us[i] -= 1 * "b"_us[j]));
  REQUIRE(prtcl::expr::_urd("a"_us[i] *= 1 * "b"_us[j]));
  REQUIRE(prtcl::expr::_urd("a"_us[i] /= 1 * "b"_us[j]));
}
