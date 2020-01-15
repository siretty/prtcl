#include <prtcl/gt/ast.hpp>
#include <prtcl/gt/dsl_to_ast.hpp>

#include <iostream>

int main(int, char **) {
  prtcl::gt::ast::collection co{"symplectic_euler"};

  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;

  auto time_step = gr_field("time_step", {});
  auto x = vr_field("position", {0});
  auto v = vr_field("velocity", {0});
  auto a = vr_field("acceleration", {0});

  co.add_child(prtcl::gt::dsl_to_ast(                    //
                   procedure(                            //
                       "advect_symplectic_euler",        //
                       foreach_particle(                 //
                           if_group_type(                //
                               "fluid",                  //
                               v[i] += time_step * a[i], //
                               x[i] += time_step * v[i]  //
                               )                         //
                           )                             //
                       )                                 //
                   )
                   .release());

  prtcl::gt::ast::cpp_openmp_printer{std::cout}(&co);
}
