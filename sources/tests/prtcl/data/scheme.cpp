#include "prtcl/tag/kind.hpp"
#include "prtcl/tag/type.hpp"
#include <catch.hpp>

#include <prtcl/data/scheme.hpp>

TEST_CASE("prtcl/data/scheme", "[prtcl][data][scheme]") {
  namespace tag = prtcl::tag;

  prtcl::data::scheme<float, 3> s;

  { // global scalar
    auto &gss = s.get(tag::type::scalar{});
    REQUIRE(!gss.has("gs"));
    gss.add("gs");
    REQUIRE(gss.has("gs"));
  }

  { // global vector
    auto &gsv = s.get(tag::type::vector{});
    REQUIRE(!gsv.has("gv"));
    gsv.add("gv");
    REQUIRE(gsv.has("gv"));
  }

  { // global matrix
    auto &gsm = s.get(tag::type::matrix{});
    REQUIRE(!gsm.has("gm"));
    gsm.add("gm");
    REQUIRE(gsm.has("gm"));
  }

  { // group
    REQUIRE(0 == s.get_group_count());
    REQUIRE(!s.has_group("g0"));
    auto &g0 = s.add_group("g0");
    REQUIRE(s.has_group("g0"));
    REQUIRE(1 == s.get_group_count());

    auto &g0uss = g0.get(tag::kind::uniform{}, tag::type::scalar{});
    REQUIRE(!g0uss.has("us"));
    g0uss.add("us");
    REQUIRE(g0uss.has("us"));
  }
}
