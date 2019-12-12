#include <catch.hpp>

#include <prtcl/data/openmp/scheme.hpp>

TEST_CASE("prtcl/data/openmp/scheme", "[prtcl][data][openmp][scheme]") {
  namespace tag = prtcl::tag;

  prtcl::data::scheme<float, 3> s_data;
  s_data.get(tag::type::scalar{}).add("gs");
  s_data.get(tag::type::vector{}).add("gv");
  s_data.get(tag::type::matrix{}).add("gm");
  s_data.add_group("g0");

  prtcl::data::openmp::scheme s{s_data};
  REQUIRE(1 == s.get(tag::type::scalar{}).field_count());
  REQUIRE(1 == s.get(tag::type::vector{}).field_count());
  REQUIRE(1 == s.get(tag::type::matrix{}).field_count());
  REQUIRE(1 == s.get_group_count());
}
