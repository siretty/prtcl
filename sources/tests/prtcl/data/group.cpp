#include <catch.hpp>

#include <prtcl/data/group.hpp>

TEST_CASE("prtcl/data/group", "[prtcl][data][group]") {
  namespace tag = prtcl::tag;

  prtcl::data::group<float, 3> g;

  REQUIRE(0 == g.size());
  g.resize(10);
  REQUIRE(10 == g.size());

  { // uniform scalar
    auto &us = g.get(tag::uniform{}, tag::scalar{});
    REQUIRE(!us.has("us"));
    us.add("us");
    REQUIRE(us.has("us"));
  }

  REQUIRE(!g.has_uniform_vector("uv"));
  g.add_uniform_vector("uv");
  REQUIRE(g.has_uniform_vector("uv"));

  REQUIRE(!g.has_uniform_matrix("um"));
  g.add_uniform_matrix("um");
  REQUIRE(g.has_uniform_matrix("um"));

  REQUIRE(!g.has_varying_scalar("vs"));
  auto &vs = g.add_varying_scalar("vs");
  REQUIRE(g.has_varying_scalar("vs"));
  REQUIRE(10 == vs.size());

  REQUIRE(!g.has_varying_vector("vv"));
  auto &vv = g.add_varying_vector("vv");
  REQUIRE(g.has_varying_vector("vv"));
  REQUIRE(10 == vv.size());

  REQUIRE(!g.has_varying_matrix("vm"));
  auto &vm = g.add_varying_matrix("vm");
  REQUIRE(g.has_varying_matrix("vm"));
  REQUIRE(10 == vm.size());

  g.resize(20);
  REQUIRE(20 == g.size());
  REQUIRE(20 == vs.size());
  REQUIRE(20 == vv.size());
  REQUIRE(20 == vm.size());
}
