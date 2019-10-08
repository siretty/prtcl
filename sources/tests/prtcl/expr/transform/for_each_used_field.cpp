#include <catch.hpp>

#include <prtcl/expr/terminal/active_index.hpp>
#include <prtcl/expr/terminal/neighbour_index.hpp>
#include <prtcl/expr/terminal/uniform_scalar.hpp>
#include <prtcl/expr/terminal/uniform_vector.hpp>
#include <prtcl/expr/terminal/varying_scalar.hpp>
#include <prtcl/expr/terminal/varying_vector.hpp>
#include <prtcl/expr/transform/for_each_used_field.hpp>

#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

TEST_CASE("prtcl::expr::transform::find_used_fields", "[prtcl][expr]") {
  using namespace prtcl::expr;

  active_index_term i;
  neighbour_index_term j;

  uniform_scalar_term<std::string> usi{"usi"}, usj{"usj"};
  varying_scalar_term<std::string> vsi{"vsi"}, vsj{"vsj"};

  uniform_vector_term<std::string> uvi{"uvi"}, uvj{"uvj"};
  varying_vector_term<std::string> vvi{"vvi"}, vvj{"vvj"};

  auto scalar_expr = boost::proto::deep_copy(usi[i] * usj[j] + vsi[i] * vsj[j]);
  auto vector_expr = boost::proto::deep_copy(uvi[i] * uvj[j] + vvi[i] * vvj[j]);

  std::unordered_set<std::string> names;

  // -----

  SECTION("active uniform scalar") {
    for_each_used_field<UniformScalarTerm, ActiveIndexTerm>(
        scalar_expr, [&names](auto &v) { names.insert(v.value); });

    REQUIRE(1 == names.size());
    REQUIRE(names.find("usi") != names.end());
  }

  SECTION("active varying scalar") {
    for_each_used_field<VaryingScalarTerm, ActiveIndexTerm>(
        scalar_expr, [&names](auto &v) { names.insert(v.value); });

    REQUIRE(1 == names.size());
    REQUIRE(names.find("vsi") != names.end());
  }

  SECTION("neighbour uniform scalar") {
    for_each_used_field<UniformScalarTerm, NeighbourIndexTerm>(
        scalar_expr, [&names](auto &v) { names.insert(v.value); });

    REQUIRE(1 == names.size());
    REQUIRE(names.find("usj") != names.end());
  }

  SECTION("neighbour varying scalar") {
    for_each_used_field<VaryingScalarTerm, NeighbourIndexTerm>(
        scalar_expr, [&names](auto &v) { names.insert(v.value); });

    REQUIRE(1 == names.size());
    REQUIRE(names.find("vsj") != names.end());
  }

  // -----

  SECTION("active uniform vector") {
    for_each_used_field<UniformVectorTerm, ActiveIndexTerm>(
        vector_expr, [&names](auto &v) { names.insert(v.value); });

    REQUIRE(1 == names.size());
    REQUIRE(names.find("uvi") != names.end());
  }

  SECTION("active varying vector") {
    for_each_used_field<VaryingVectorTerm, ActiveIndexTerm>(
        vector_expr, [&names](auto &v) { names.insert(v.value); });

    REQUIRE(1 == names.size());
    REQUIRE(names.find("vvi") != names.end());
  }

  SECTION("neighbour uniform vector") {
    for_each_used_field<UniformVectorTerm, NeighbourIndexTerm>(
        vector_expr, [&names](auto &v) { names.insert(v.value); });

    REQUIRE(1 == names.size());
    REQUIRE(names.find("uvj") != names.end());
  }

  SECTION("neighbour varying vector") {
    for_each_used_field<VaryingVectorTerm, NeighbourIndexTerm>(
        vector_expr, [&names](auto &v) { names.insert(v.value); });

    REQUIRE(1 == names.size());
    REQUIRE(names.find("vvj") != names.end());
  }
}
