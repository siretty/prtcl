#include <prtcl/gt/dsl.hpp>
#include <prtcl/gt/schemes_registry.hpp>

PRTCL_GT_START_REGISTER_SCHEME_PROCEDURES(test)

using namespace prtcl::gt::dsl::language;
using namespace prtcl::gt::dsl::generic_indices;

auto gpc = gi_field("global_particle_count", {});

registry.scheme_procedures(
    "test",                        //
    procedure(                     //
        "test_counting_particles", //
        foreach_particle(          //
            if_group_type(         //
                "particles",       //
                gpc += 1           //
                )                  //
            )                      //
        ));

auto gnc = gi_field("global_neighbor_count", {});

registry.scheme_procedures(
    "test",                          //
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
);

PRTCL_GT_CLOSE_REGISTER_SCHEME_PROCEDURES
