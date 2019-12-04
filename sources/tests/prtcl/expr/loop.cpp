#include <catch.hpp>

#include <prtcl/expr/eq.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/meta/format_cxx_type.hpp>

#include <boost/yap/print.hpp>

TEST_CASE("prtcl/expr/loop", "[prtcl][expr][loop]") {
  using namespace prtcl::expr_literals;
  using namespace prtcl::expr_language;

  prtcl::tag::group::active i;
  prtcl::tag::group::passive j;

  auto select_fluid = [](auto g) { return g.has_flag("fluid"); };

  auto l = foreach_particle(                                 //
      only(select_fluid)(                                    //
          "a"_vs[i] = "b"_us[i],                             //
          "c"_vv[i] = "d"_uv[i],                             //
          "i"_us[i] += "j"_vs[i],                            //
          "k"_gs += "l"_vs[i],                               //
          foreach_neighbour(                                 //
              only(select_fluid)(                            //
                  "e"_vs[i] += "f"_vs[j],                    //
                  "g"_vv[i] += "h"_vv[j],                    //
                  "m"_gv = reduce_max("n"_us[i] * "o"_vv[j]) //
                  )                                          //
              )                                              //
          )                                                  //
  );

  display_cxx_type(l, std::cout);
  boost::yap::print(std::cout, l);
}

//
//  foreach_particle(
//    only(select_fluid)(
//      foreach_neighbour(
//        only(select_fluid)(
//          rho[i] += m[j] * W(x[i], x[j])
//        ),
//        only(select_boundary)(
//          rho[i] += V[j] * rho0[i] * W(x[i], x[j])
//        )
//      ),
//      p[i] = kappa[i] * max(0, rho[i] / rho0[i] - 1)
//    )
//  )
//
//  foreach_particle(
//    only(select_dynamic)(
//      x[i] += dt * v[i],
//      v[i] += dt * a[i]
//    ),
//    only(select_static)(
//      a[i] += b[i]
//    )
//  )
//
