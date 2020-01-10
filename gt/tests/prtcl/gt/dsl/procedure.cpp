#include <catch.hpp>

#include <prtcl/gt/ast.hpp>
#include <prtcl/gt/dsl.hpp>
#include <prtcl/gt/dsl_to_ast.hpp>

#include <iostream>

TEST_CASE("prtcl/gt/dsl/procedure", "[prtcl]") {
  using namespace ::prtcl::gt::dsl::language;
  using namespace ::prtcl::gt::dsl::monaghan_indices; // a, b
  using namespace ::prtcl::gt::dsl::kernel_shorthand; // W, dW

  auto const g = gr_field("gravity", {0});
  auto const x = vr_field("position", {0});

  auto proc = procedure(
      "test",                                            //
      g = ones({0}),                                     //
      foreach_particle(                                  //
          if_group_type(                                 //
              "some_type",                               //
              foreach_neighbor(                          //
                  if_group_type(                         //
                      "other_type",                      //
                      x[a] = zeros({0}),                 //
                      x[a] += dot(x[a], x[a]),           //
                      x[a] /= norm(ones({0})) * dW(x[a]) //
                      )                                  //
                  )                                      //
              )                                          //
          )                                              //
  );

  auto ast = ::prtcl::gt::dsl_to_ast(proc);

  ::prtcl::gt::ast::ast_latex_printer{std::cerr}(ast.get());
}
