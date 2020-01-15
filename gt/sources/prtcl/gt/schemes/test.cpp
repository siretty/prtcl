#include <prtcl/gt/ast.hpp>
#include <prtcl/gt/dsl_to_ast.hpp>

#include <iostream>

int main(int, char **) {
  prtcl::gt::ast::collection co{"test"};

  using namespace prtcl::gt::dsl::language;
  using namespace prtcl::gt::dsl::generic_indices;

  auto gpc = gi_field("global_particle_count", {});

  co.add_child(prtcl::gt::dsl_to_ast(             //
                   procedure(                     //
                       "test_counting_particles", //
                       foreach_particle(          //
                           if_group_type(         //
                               "particles",       //
                               gpc += 1           //
                               )                  //
                           )                      //
                       )                          //
                   )
                   .release());

  auto gnc = gi_field("global_neighbor_count", {});

  co.add_child(prtcl::gt::dsl_to_ast(               //
                   procedure(                       //
                       "test_counting_neighbors",   //
                       foreach_particle(            //
                           if_group_type(           //
                               "particles",         //
                               foreach_neighbor(    //
                                   if_group_type(   //
                                       "neighbors", //
                                       gnc += 1     //
                                       )            //
                                   )                //
                               )                    //
                           )                        //
                       )                            //
                   )
                   .release());

  prtcl::gt::ast::cpp_openmp_printer{std::cout}(&co);
}
