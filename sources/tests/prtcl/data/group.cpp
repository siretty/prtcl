#include "prtcl/tag/kind.hpp"
#include <catch.hpp>

#include <prtcl/data/group.hpp>

TEST_CASE("prtcl/data/group", "[prtcl][data][group]") {
  namespace tag = prtcl::tag;

  prtcl::data::group<float, 3> g;

  REQUIRE(0 == g.size());
  g.resize(10);
  REQUIRE(10 == g.size());

  { // uniform scalar
    auto &uss = g.get(tag::kind::uniform{}, tag::type::scalar{});
    REQUIRE(!uss.has("us"));
    uss.add("us");
    REQUIRE(uss.has("us"));
  }

  { // uniform vector
    auto &uvs = g.get(tag::kind::uniform{}, tag::type::vector{});
    REQUIRE(!uvs.has("uv"));
    uvs.add("uv");
    REQUIRE(uvs.has("uv"));
  }

  { // uniform matrix
    auto &ums = g.get(tag::kind::uniform{}, tag::type::matrix{});
    REQUIRE(!ums.has("um"));
    ums.add("um");
    REQUIRE(ums.has("um"));
  }

  SECTION("resizing affects varyings") {
    // varying scalar
    auto &vss = g.get(tag::kind::varying{}, tag::type::scalar{});
    REQUIRE(!vss.has("vs"));
    auto &vs = vss.add("vs");
    REQUIRE(vss.has("vs"));
    REQUIRE(10 == vs.size());

    // varying vector
    auto &vvs = g.get(tag::kind::varying{}, tag::type::vector{});
    REQUIRE(!vvs.has("vv"));
    auto &vv = vvs.add("vv");
    REQUIRE(vvs.has("vv"));
    REQUIRE(10 == vv.size());

    // varying matrix
    auto &vms = g.get(tag::kind::varying{}, tag::type::matrix{});
    REQUIRE(!vms.has("vm"));
    auto &vm = vms.add("vm");
    REQUIRE(vms.has("vm"));
    REQUIRE(10 == vm.size());

    g.resize(20);
    REQUIRE(20 == g.size());
    REQUIRE(20 == vs.size());
    REQUIRE(20 == vv.size());
    REQUIRE(20 == vm.size());
  }
}
