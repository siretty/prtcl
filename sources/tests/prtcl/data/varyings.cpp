#include <catch.hpp>

#include <prtcl/data/varyings.hpp>

TEST_CASE("prtcl/data/varyings", "[prtcl][data][varyings]") {
  prtcl::data::varyings_t<float> vs;

  REQUIRE(!vs.has("a"));

  vs.add("a");
  REQUIRE(vs.has("a"));
}
