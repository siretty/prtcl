#include <catch.hpp>

#include <prtcl/data/openmp/group.hpp>

TEST_CASE("prtcl/data/openmp/group", "[prtcl][data][openmp][group]") {
  namespace tag = prtcl::tag;

  prtcl::data::group<float, 3> g_data;
  g_data.resize(10);
  g_data.get(tag::kind::uniform{}, tag::type::scalar{}).add("us");
  g_data.get(tag::kind::uniform{}, tag::type::vector{}).add("uv");
  g_data.get(tag::kind::uniform{}, tag::type::matrix{}).add("um");
  g_data.get(tag::kind::varying{}, tag::type::scalar{}).add("vs");
  g_data.get(tag::kind::varying{}, tag::type::vector{}).add("vv");
  g_data.get(tag::kind::varying{}, tag::type::matrix{}).add("vm");

  prtcl::data::openmp::group g{g_data};
  REQUIRE(10 == g.size());
  REQUIRE(1 == g.get(tag::kind::uniform{}, tag::type::scalar{}).field_count());
  REQUIRE(1 == g.get(tag::kind::uniform{}, tag::type::vector{}).field_count());
  REQUIRE(1 == g.get(tag::kind::uniform{}, tag::type::matrix{}).field_count());
  REQUIRE(1 == g.get(tag::kind::varying{}, tag::type::scalar{}).field_count());
  REQUIRE(1 == g.get(tag::kind::varying{}, tag::type::vector{}).field_count());
  REQUIRE(1 == g.get(tag::kind::varying{}, tag::type::matrix{}).field_count());
  REQUIRE(10 == g.get(tag::kind::varying{}, tag::type::scalar{})[0].size());
  REQUIRE(10 == g.get(tag::kind::varying{}, tag::type::vector{})[0].size());
  REQUIRE(10 == g.get(tag::kind::varying{}, tag::type::matrix{})[0].size());
}
