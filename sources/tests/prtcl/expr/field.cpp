#include <catch.hpp>

#include <prtcl/expr/field.hpp>

TEST_CASE("prtcl/expr/field", "[prtcl][expr][field]") {
  using namespace prtcl::expr_literals;

  SECTION("instantiation with user defined literals") {
    auto gs = "gs"_gsf;
    REQUIRE(gs.kind_tag == prtcl::tag::kind::global{});
    REQUIRE(gs.type_tag == prtcl::tag::type::scalar{});
    REQUIRE(gs.value == "gs");

    auto gv = "gv"_gvf;
    REQUIRE(gv.kind_tag == prtcl::tag::kind::global{});
    REQUIRE(gv.type_tag == prtcl::tag::type::vector{});
    REQUIRE(gv.value == "gv");

    auto gm = "gm"_gmf;
    REQUIRE(gm.kind_tag == prtcl::tag::kind::global{});
    REQUIRE(gm.type_tag == prtcl::tag::type::matrix{});
    REQUIRE(gm.value == "gm");
  }

  SECTION("literal op field") {
    auto e0 = 1 * "a"_gs;
    REQUIRE(boost::yap::is_expr<decltype(e0)>::value);
    REQUIRE(!prtcl::expr::is_field_v<decltype(e0.left().value())>);
    REQUIRE(prtcl::expr::is_field_v<decltype(e0.right().value())>);
    REQUIRE(e0.right().value().kind_tag == prtcl::tag::kind::global{});
    REQUIRE(e0.right().value().type_tag == prtcl::tag::type::scalar{});
    REQUIRE(e0.right().value().value == "a");

    auto e1 = "a"_gs * 1;
    REQUIRE(boost::yap::is_expr<decltype(e1)>::value);
    REQUIRE(prtcl::expr::is_field_v<decltype(e1.left().value())>);
    REQUIRE(e1.left().value().kind_tag == prtcl::tag::kind::global{});
    REQUIRE(e1.left().value().type_tag == prtcl::tag::type::scalar{});
    REQUIRE(e1.left().value().value == "a");
    REQUIRE(!prtcl::expr::is_field_v<decltype(e1.right().value())>);

    auto e2 = "a"_gs * "b"_us;
    REQUIRE(boost::yap::is_expr<decltype(e2)>::value);
    REQUIRE(prtcl::expr::is_field_v<decltype(e2.left().value())>);
    REQUIRE(e2.left().value().kind_tag == prtcl::tag::kind::global{});
    REQUIRE(e2.left().value().type_tag == prtcl::tag::type::scalar{});
    REQUIRE(e2.left().value().value == "a");
    REQUIRE(prtcl::expr::is_field_v<decltype(e2.right().value())>);
    REQUIRE(e2.right().value().kind_tag == prtcl::tag::kind::uniform{});
    REQUIRE(e2.right().value().type_tag == prtcl::tag::type::scalar{});
    REQUIRE(e2.right().value().value == "b");
  }
}
