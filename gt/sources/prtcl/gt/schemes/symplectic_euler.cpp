#include <prtcl/gt/dsl.hpp>
#include <prtcl/gt/schemes_registry.hpp>

PRTCL_GT_START_REGISTER_SCHEME_PROCEDURES(symplectic_euler)

using namespace prtcl::gt::dsl::language;
using namespace prtcl::gt::dsl::generic_indices;

auto time_step = gr_field("time_step", {});
auto x = vr_field("position", {0});
auto v = vr_field("velocity", {0});
auto a = vr_field("acceleration", {0});

registry.scheme_procedures(
    "symplectic_euler",                   //
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
);

PRTCL_GT_CLOSE_REGISTER_SCHEME_PROCEDURES
