#include <catch.hpp>

#include <prtcl/data/uniforms.hpp>

TEST_CASE("prtcl/data/uniforms", "[prtcl][data][uniforms]") {
  prtcl::data::uniforms_t<float> us;

  REQUIRE(!us.has("a"));

  us.add("a");
  REQUIRE(us.has("a"));
}
