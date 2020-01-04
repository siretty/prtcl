#include "prtcl/tag/group.hpp"
#include <catch.hpp>

#include <prtcl/openmp_source_generator.hpp>

#include <iostream>
#include <sstream>

TEST_CASE("prtcl/openmp_source_generator", "[prtcl][openmp_source_generator]") {
  using namespace ::prtcl::expr_language;
  using namespace ::prtcl::expr_literals;

  constexpr ::prtcl::tag::group::active i;
  constexpr ::prtcl::tag::group::passive j;

  std::ostringstream stream;

  ::prtcl::generate_openmp_source(
      stream, "TEST",
      make_named_section(                         //
          "section_a",                            //
          "gs"_gs = 0,                            //
          foreach_particle(                       //
              only("fluid")(                      //
                  "a"_vs[i] = 0,                  //
                  foreach_neighbour(              //
                      only("fluid")(              //
                          "a"_vs[i] += "b"_vs[j], //
                          "d"_us[i] += "b"_vs[j], //
                          "gs"_gs -= 1            //
                          ),                      //
                      only("boundary")(           //
                          "a"_vs[i] += "c"_vs[j], //
                          "d"_us[i] += "b"_vs[j]  //
                          )                       //
                      )                           //
                  )                               //
              ),                                  //
          "gs"_gs += 1                            //
          ),                                      //
      make_named_section(                         //
          "section_b",                            //
          "gs"_gs += 2,                           //
          foreach_particle(                       //
              only("fluid")(                      //
                  "a"_vs[i] = 0,                  //
                  foreach_neighbour(              //
                      only("fluid")(              //
                          "a"_vs[i] += "b"_vs[j]  //
                          ),                      //
                      only("boundary")(           //
                          "a"_vs[i] += "c"_vs[j]  //
                          )                       //
                      )                           //
                  )                               //
              ),                                  //
          "gs"_gs += 3                            //
          )                                       //
  );

  std::cerr << stream.str() << std::endl;
}
