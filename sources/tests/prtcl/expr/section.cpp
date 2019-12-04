#include <catch.hpp>

#include <prtcl/expr/call.hpp>
#include <prtcl/expr/eq.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/expr/print.hpp>
#include <prtcl/expr/section.hpp>
#include <prtcl/meta/format_cxx_type.hpp>

TEST_CASE("prtcl/expr/section", "[prtcl][expr][section]") {
  using namespace prtcl::expr_literals;
  using namespace prtcl::expr_language;

  prtcl::tag::group::active i;
  prtcl::tag::group::passive j;

  auto select_fluid = [](auto g) { return g.has_flag("fluid"); };

  auto s = make_named_section(                                      //
      "test section",                                               //
      "i"_gs = 0,                                                   //
      foreach_particle(                                             //
          only(select_fluid)(                                       //
              "a"_vs[i] = dot("b"_uv[i], "c"_vv[i]),                //
              "c"_vv[i] = "d"_uv[i],                                //
              "k"_gs += "l"_vs[i],                                  //
              foreach_neighbour(                                    //
                  only(select_fluid)(                               //
                      "e"_vs[i] += "f"_vs[j],                       //
                      "g"_vv[i] += "h"_vv[j],                       //
                      "m"_uv[i] = reduce_max("n"_us[i] * "o"_vv[j]) //
                      )                                             //
                  )                                                 //
              )                                                     //
          ),                                                        //
      "j"_gs += 1                                                   //
  );

  display_cxx_type(s, std::cout);
  boost::yap::print(std::cout, s);

  prtcl::expr::print(std::cout, s);
}
