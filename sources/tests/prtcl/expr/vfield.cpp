#include <catch.hpp>

#include <prtcl/expr/vfield.hpp>

TEST_CASE("prtcl/expr/vfield", "[prtcl][expr][vfield]") {
  using namespace prtcl::expr::literals;

  using prtcl::tag::kind::varying;
  using prtcl::tag::type::matrix;
  using prtcl::tag::type::scalar;
  using prtcl::tag::type::vector;

  auto vs = "vs"_vs;
  REQUIRE(std::is_same<decltype(vs)::kind_tag, varying>::value);
  REQUIRE(std::is_same<decltype(vs)::type_tag, scalar>::value);
  REQUIRE(vs.value == "vs");

  auto vv = "vv"_vv;
  REQUIRE(std::is_same<decltype(vv)::kind_tag, varying>::value);
  REQUIRE(std::is_same<decltype(vv)::type_tag, vector>::value);
  REQUIRE(vv.value == "vv");

  auto vm = "vm"_vm;
  REQUIRE(std::is_same<decltype(vm)::kind_tag, varying>::value);
  REQUIRE(std::is_same<decltype(vm)::type_tag, matrix>::value);
  REQUIRE(vm.value == "vm");
}
