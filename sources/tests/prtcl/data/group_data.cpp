#include <catch.hpp>

#include <prtcl/data/group_data.hpp>

TEST_CASE("prtcl::group_data", "[prtcl]") {
  using namespace prtcl;

  using T = float;
  constexpr size_t N = 3;

  group_data<T, N> data;
  REQUIRE(0 == data.size());

  { // unknown names
    auto vs_na = data.get_varying_scalar("not available");
    REQUIRE(!vs_na.has_value());

    auto vv_na = data.get_varying_vector("not available");
    REQUIRE(!vv_na.has_value());

    auto us_na = data.get_uniform_scalar_index("not available");
    REQUIRE(!us_na.has_value());

    auto uv_na = data.get_uniform_vector_index("not available");
    REQUIRE(!uv_na.has_value());
  }

  data.resize(10);
  REQUIRE(10 == data.size());

  { // known names
    data.add_varying_scalar("a");
    auto vs_a = data.get_varying_scalar("a");
    REQUIRE(vs_a.has_value());
    REQUIRE(10 == vs_a->size());

    data.add_varying_vector("a");
    auto vv_a = data.get_varying_vector("a");
    REQUIRE(vv_a.has_value());
    REQUIRE(10 == vv_a->size());

    data.add_uniform_scalar("a");
    auto us_a = data.get_uniform_scalar_index("a");
    REQUIRE(us_a.has_value());

    data.add_uniform_vector("a");
    auto uv_a = data.get_uniform_vector_index("a");
    REQUIRE(uv_a.has_value());

    data.resize(20);
    REQUIRE(20 == data.size());
    REQUIRE(20 == vs_a->size());
    REQUIRE(20 == vv_a->size());
  }
}

// vim: set foldmethod=marker:
