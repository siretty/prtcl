#include <catch.hpp>

#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/group.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/scheme/openmp/compiler.hpp>

#include <string>

TEST_CASE("prtcl/scheme/openmp/compiler", "[prtcl][scheme][openmp][compiler]") {
  namespace tag = prtcl::tag;

  prtcl::expr::active_group a;
  prtcl::expr::passive_group p;

  prtcl::expr::gscalar<std::string> gs{{"gs"}};
  prtcl::expr::gvector<std::string> gv{{"gv"}};
  prtcl::expr::uscalar<std::string> us{{"us"}};
  prtcl::expr::uvector<std::string> uv{{"uv"}};
  prtcl::expr::vscalar<std::string> vs{{"vs"}};
  prtcl::expr::vvector<std::string> vv{{"vv"}};

  prtcl::expr::vvector<std::string> r{{"r"}};

  auto foreach_a =
      prtcl::expr::make_loop([](auto &g) { return g.has_flag("a"); });

  auto foreach_p =
      prtcl::expr::make_loop([](auto &g) { return g.has_flag("p"); });

  auto expr = foreach_a(
      vs[a] += gs, foreach_p(r[a] = gs * gv + us[a] * uv[p] + vs[a] * vv[p]));

  prtcl::data::scheme<float, 3> scheme;

  { // add required fields (TODO: extract from the expressions)
    scheme.get(tag::global{}, tag::scalar{}).add("gs");
    scheme.get(tag::global{}, tag::vector{}).add("gv");

    auto &ga = scheme.add_group("ga");
    ga.add_flag("a");
    ga.get(tag::varying{}, tag::vector{}).add("r");
    ga.get(tag::uniform{}, tag::scalar{}).add("us");
    ga.get(tag::varying{}, tag::scalar{}).add("vs");

    auto &gp = scheme.add_group("gp");
    gp.add_flag("p");
    gp.get(tag::uniform{}, tag::vector{}).add("uv");
    gp.get(tag::varying{}, tag::vector{}).add("vv");
  }

  prtcl::scheme::openmp::compiler<float, 3> compile{scheme};
  compile(expr);
}
