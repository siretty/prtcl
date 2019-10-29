#include <catch.hpp>

#include <prtcl/data/openmp/group.hpp>

TEST_CASE("prtcl/data/openmp/group", "[prtcl][data][openmp][group]") {
  namespace tag = prtcl::tag;

  prtcl::data::group<float, 3> g_data;
  g_data.resize(10);
  g_data.add_uniform_scalar("us");
  g_data.add_uniform_vector("uv");
  g_data.add_uniform_matrix("um");
  g_data.add_varying_scalar("vs");
  g_data.add_varying_vector("vv");
  g_data.add_varying_matrix("vm");

  prtcl::data::openmp::group g{g_data};
  REQUIRE(1 == g.get(tag::uniform{}, tag::scalar{}).field_count());
  REQUIRE(1 == g.get_uniform_scalar_count());
  REQUIRE(1 == g.get_uniform_vector_count());
  REQUIRE(1 == g.get_uniform_matrix_count());
  REQUIRE(1 == g.get_varying_scalar_count());
  REQUIRE(1 == g.get_varying_vector_count());
  REQUIRE(1 == g.get_varying_matrix_count());
  REQUIRE(10 == g.get_varying_scalar(0).size());
  REQUIRE(10 == g.get_varying_vector(0).size());
  REQUIRE(10 == g.get_varying_matrix(0).size());
  REQUIRE(10 == g.size());
}
