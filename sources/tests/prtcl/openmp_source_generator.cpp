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

  auto section = make_named_section(         //
      "Test",                                //
      "gs"_gs = 0,                           //
      foreach_particle(                      //
          only("fluid")(                     //
              "a"_vs[i] = 0,                 //
              foreach_neighbour(             //
                  only("fluid")(             //
                      "a"_vs[i] += "b"_vs[j] //
                      ),                     //
                  only("boundary")(          //
                      "a"_vs[i] += "c"_vs[j] //
                      )                      //
                  )                          //
              )                              //
          ),                                 //
      "gs"_gs += 1                           //
  );

  ::prtcl::generate_openmp_source(stream, "TEST", section);
  std::cerr << stream.str() << std::endl;
}
