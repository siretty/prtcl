#include <catch.hpp>

#include <prtcl/data/openmp/scheme.hpp>

TEST_CASE("prtcl/data/openmp/scheme", "[prtcl][data][openmp][scheme]") {
  prtcl::data::scheme<float, 3> s_data;
  s_data.add_global_scalar("gs");
  s_data.add_global_vector("gv");
  s_data.add_global_matrix("gm");
  s_data.add_group("g0");

  prtcl::data::openmp::scheme s{s_data};
  REQUIRE(1 == s.get_global_scalar_count());
  REQUIRE(1 == s.get_global_vector_count());
  REQUIRE(1 == s.get_global_matrix_count());
  REQUIRE(1 == s.get_group_count());
}
