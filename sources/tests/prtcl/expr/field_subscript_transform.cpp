#include <catch.hpp>

#include <prtcl/expr/field.hpp>
#include <prtcl/expr/field_subscript_transform.hpp>
#include <prtcl/expr/group.hpp>
#include <prtcl/meta/is_any_of.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tags.hpp>

#include <boost/yap/yap.hpp>

TEST_CASE("prtcl/expr/bind_subscript",
          "[prtcl][expr][transform][bind_subscript]") {
  using namespace prtcl;

  expr::group_term<tag::active> a;
  expr::group_term<tag::passive> p;

  expr::uscalar<int> us{{1234}};
  expr::vvector<int> vv{{5678}};

  auto transform = expr::field_subscript_transform{};

  auto expr = us[a] * vv[p];
  auto transformed_expr = boost::yap::transform(expr, transform);

  REQUIRE(1234 == transformed_expr.left().value().value);
  REQUIRE(is_any_of_v<typename remove_cvref_t<decltype(
                          transformed_expr.left().value())>::group_tag,
                      tag::active>);

  REQUIRE(5678 == transformed_expr.right().value().value);
  REQUIRE(is_any_of_v<typename remove_cvref_t<decltype(
                          transformed_expr.right().value())>::group_tag,
                      tag::passive>);
}
