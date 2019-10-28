#include <catch.hpp>

#include <prtcl/expr/field.hpp>
#include <prtcl/expr/field_value_transform.hpp>
#include <prtcl/meta/overload.hpp>
#include <prtcl/tags.hpp>

#include <iostream>
#include <string>

#include <boost/yap/yap.hpp>

TEST_CASE("prtcl/expr/field_value_transform",
          "[prtcl][expr][transform][field_value_transform]") {
  using namespace prtcl;

  expr::field_term<tag::uniform, tag::scalar, tag::active, tag::read_write, int>
      us{{1234}};
  expr::field_term<tag::varying, tag::vector, tag::passive, tag::read_write,
                   int>
      vv{{5678}};

  auto transform = expr::make_field_value_transform(
      meta::overload{[](tag::uniform, auto, auto, auto, int value) {
                       return "uniform(" + std::to_string(value) + ")";
                     },
                     [](tag::varying, auto, auto, auto, int value) {
                       return "varying(" + std::to_string(value) + ")";
                     }});

  auto expr = us * vv;
  auto transformed_expr = boost::yap::transform(expr, transform);

  REQUIRE("uniform(1234)" == transformed_expr.left().value().value);
  REQUIRE("varying(5678)" == transformed_expr.right().value().value);
}
