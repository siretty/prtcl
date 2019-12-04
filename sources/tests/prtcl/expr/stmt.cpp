#include <catch.hpp>

#include <prtcl/expr/stmt.hpp>

TEST_CASE("prtcl/expr/stmt", "[prtcl][expr][stmt]") {
  using namespace prtcl::expr::literals;

  auto a = "a"_gs, b = "b"_gs, c = "c"_gs;

  prtcl::expr::nl_stmt(a = b * c);
}
