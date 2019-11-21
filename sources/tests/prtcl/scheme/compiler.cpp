#include "prtcl/scheme/compiler/eq.hpp"
#include <catch.hpp>

#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/call.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/group.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/meta/overload.hpp>
#include <prtcl/scheme/compiler.hpp>

#include <string>

TEST_CASE("prtcl/scheme/compiler", "[prtcl][scheme][compiler]") {
  namespace tag = prtcl::tag;

  prtcl::expr::active_group a;
  prtcl::expr::passive_group p;

  prtcl::expr::gscalar<std::string> gs{{"gs"}};
  prtcl::expr::gvector<std::string> gv1{{"gv1"}};
  prtcl::expr::gvector<std::string> gv2{{"gv2"}};
  prtcl::expr::uscalar<std::string> us{{"us"}};
  prtcl::expr::uvector<std::string> uv{{"uv"}};
  prtcl::expr::vscalar<std::string> vs{{"vs"}};
  prtcl::expr::vvector<std::string> vv{{"vv"}};

  prtcl::expr::call_term<tag::dot> dot;

  prtcl::expr::vvector<std::string> r{{"r"}};

  auto foreach_a =
      prtcl::expr::make_loop([](auto &g) { return g.has_flag("a"); });

  auto foreach_p =
      prtcl::expr::make_loop([](auto &g) { return g.has_flag("p"); });

  auto expr = foreach_a(
      gs += dot(Eigen::Vector3f{1, 1, 1}, Eigen::Vector3f{-1, -2, 4}),
      vs[a] = gs,
      foreach_p(r[a] += gs * (gv1 + gv2) + us[a] * uv[p] + vs[a] * vv[p],
                prtcl::scheme::make_min_reduce_eq(gs, us[a] + us[p] * vs[p])));

  prtcl::data::scheme<float, 3> scheme;

  { // add required fields (TODO: extract from the expressions)
    scheme.get(tag::global{}, tag::scalar{}).add("gs") = 1;
    scheme.get(tag::global{}, tag::vector{}).add("gv1") =
        Eigen::Vector3f{2, 3, 4};
    scheme.get(tag::global{}, tag::vector{}).add("gv2") =
        Eigen::Vector3f{5, 6, 7};

    auto &ga = scheme.add_group("ga");
    ga.add_flag("a");
    ga.get(tag::uniform{}, tag::scalar{}).add("us") = 8;
    ga.get(tag::varying{}, tag::vector{}).add("r");
    ga.get(tag::varying{}, tag::scalar{}).add("vs");
    ga.resize(2);

    auto &gp = scheme.add_group("gp");
    gp.add_flag("p");
    gp.get(tag::uniform{}, tag::vector{}).add("uv");
    gp.get(tag::varying{}, tag::vector{}).add("vv");
    gp.resize(2);
  }

  prtcl::scheme::compiler<float, 3> compile{scheme};
  auto func1 = compile(expr);

  display_cxx_type(func1, std::cout);
}
