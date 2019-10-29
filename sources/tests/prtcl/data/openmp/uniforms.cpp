#include "prtcl/data/tensors.hpp"
#include <catch.hpp>

#include <prtcl/data/openmp/uniforms.hpp>

TEST_CASE("prtcl/data/openmp/uniforms", "[prtcl][data][openmp][uniforms]") {
  prtcl::data::uniforms_t<float> us_data;
  us_data.add("a");

  prtcl::data::openmp::uniforms us{us_data};
  REQUIRE(1 == us.field_count());
}
