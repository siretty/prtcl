#include <prtcl/gt/dsl.hpp>
#include <prtcl/gt/schemes_registry.hpp>

PRTCL_GT_START_REGISTER_SCHEME_PROCEDURES(boundary)

using namespace prtcl::gt::dsl::language;
using namespace prtcl::gt::dsl::generic_indices;
using namespace prtcl::gt::dsl::kernel_shorthand;

auto const x = vr_field("position", {0});
auto const V = vr_field("volume", {});

registry.scheme_procedures(
    "boundary",                                //
    procedure(                                 //
        "compute_volume",                      //
        foreach_particle(                      //
            if_group_type(                     //
                "boundary",                    //
                V[i] = 0.0,                    //
                foreach_neighbor(              //
                    if_group_type(             //
                        "boundary",            //
                        V[i] += W(x[i] - x[j]) //
                        )                      //
                    ),                         //
                V[i] = 1 / V[i]                //
                )                              //
            )                                  //
        ));

PRTCL_GT_CLOSE_REGISTER_SCHEME_PROCEDURES
