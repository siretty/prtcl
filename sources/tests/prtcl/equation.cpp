#include <catch.hpp>

#include <prtcl/data/group_data.hpp>
#include <prtcl/equation.hpp>
#include <prtcl/expr/terminal/active_index.hpp>
#include <prtcl/expr/terminal/neighbour_index.hpp>
#include <prtcl/expr/terminal/varying_scalar.hpp>
#include <prtcl/expr/transform/for_modified_field.hpp>

#include <string>
#include <unordered_set>
#include <vector>

auto make_test_equation() {
  prtcl::expr::active_index_term i;
  prtcl::expr::neighbour_index_term j;

  prtcl::expr::varying_scalar_term<std::string> a{"a"};
  prtcl::expr::varying_scalar_term<std::string> b{"b"};
  prtcl::expr::varying_scalar_term<std::string> c{"c"};

  return prtcl::make_equation(
      prtcl::make_prepare_expressions(b[i] = 2 * a[i], c[i] = 2 * b[i]),
      prtcl::make_accumulate_expressions(a[i] += b[j], a[i] += c[j]),
      prtcl::make_finish_expressions(a[i] = 5 * a[i], c[i] = 3 * c[i]));
}

TEST_CASE("prtcl::equation", "[prtcl][expr]") {
  // using T = float;
  // constexpr size_t N = 3;

  // std::vector<prtcl::group_data<T, N>> groups;
  // groups.resize(2);

  // auto &g0 = groups[0];
  // auto &g1 = groups[1];

  auto eq = make_test_equation();

  std::unordered_set<std::string> names;

  SECTION("check prepare expressions") {
    eq.for_each_prepare_expression([&names](auto &e) {
      prtcl::expr::for_modified_field<prtcl::expr::VaryingScalarTerm,
                                      prtcl::expr::ActiveIndexTerm>(
          e, [&names](auto &v) { names.insert(v.value); });
    });

    REQUIRE(2 == names.size());
    REQUIRE(names.find("b") != names.end());
    REQUIRE(names.find("c") != names.end());
  }

  SECTION("check accumulate expressions") {
    eq.for_each_accumulate_expression([&names](auto &e) {
      prtcl::expr::for_modified_field<prtcl::expr::VaryingScalarTerm,
                                      prtcl::expr::ActiveIndexTerm>(
          e, [&names](auto &v) { names.insert(v.value); });
    });

    REQUIRE(1 == names.size());
    REQUIRE(names.find("a") != names.end());
  }

  SECTION("check finish expressions") {
    eq.for_each_finish_expression([&names](auto &e) {
      prtcl::expr::for_modified_field<prtcl::expr::VaryingScalarTerm,
                                      prtcl::expr::ActiveIndexTerm>(
          e, [&names](auto &v) { names.insert(v.value); });
    });

    REQUIRE(2 == names.size());
    REQUIRE(names.find("a") != names.end());
    REQUIRE(names.find("c") != names.end());
  }

  SECTION("check all expressions") {
    eq.for_each_expression([&names](auto &e) {
      prtcl::expr::for_modified_field<prtcl::expr::VaryingScalarTerm,
                                      prtcl::expr::ActiveIndexTerm>(
          e, [&names](auto &v) { names.insert(v.value); });
    });

    REQUIRE(3 == names.size());
    REQUIRE(names.find("a") != names.end());
    REQUIRE(names.find("b") != names.end());
    REQUIRE(names.find("c") != names.end());
  }
}
