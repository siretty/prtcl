#include <catch.hpp>

#include <prtcl/data/scheme.hpp>

TEST_CASE("prtcl/data/scheme", "[prtcl][data][scheme]") {
  prtcl::data::scheme<float, 3> s;

  REQUIRE(!s.has_global_scalar("gs"));
  s.add_global_scalar("gs");
  REQUIRE(s.has_global_scalar("gs"));

  REQUIRE(!s.has_global_vector("gv"));
  s.add_global_vector("gv");
  REQUIRE(s.has_global_vector("gv"));

  REQUIRE(!s.has_global_matrix("gm"));
  s.add_global_matrix("gm");
  REQUIRE(s.has_global_matrix("gm"));

  REQUIRE(!s.has_group("g0"));
  auto &g0 = s.add_group("g0");
  REQUIRE(s.has_group("g0"));

  REQUIRE(!g0.has_uniform_scalar("us"));
  g0.add_uniform_scalar("us");
  REQUIRE(g0.has_uniform_scalar("us"));
}
