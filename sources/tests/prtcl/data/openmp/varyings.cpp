#include "prtcl/data/tensors.hpp"
#include <catch.hpp>

#include <prtcl/data/openmp/varyings.hpp>

TEST_CASE("prtcl/data/openmp/varyings", "[prtcl][data][openmp][varyings]") {
  prtcl::data::varyings_t<float> vs_data;
  vs_data.add("a");
  ::prtcl::detail::resize_access::resize(vs_data, 10ul);

  prtcl::data::openmp::varyings vs{vs_data};
  REQUIRE(1 == vs.field_count());
  REQUIRE(10 == vs.size()); 
}

