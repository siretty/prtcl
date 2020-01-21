#include <prtcl/gt/dsl.hpp>
#include <prtcl/gt/schemes_registry.hpp>

#include <prtcl/gt/schemes/aiast12.hpp>
#include <prtcl/gt/schemes/common.hpp>

PRTCL_GT_START_REGISTER_SCHEME_PROCEDURES(sesph)

using namespace prtcl::gt::dsl::language;
using namespace prtcl::gt::dsl::generic_indices;
using namespace prtcl::gt::dsl::kernel_shorthand;

namespace common = prtcl::gt::schemes::common;
namespace aiast12 = prtcl::gt::schemes::aiast12;

auto const rho = vr_field("density", {});
auto const p = vr_field("pressure", {});

registry.scheme_procedures(
    "sesph",                                               //
    procedure(                                             //
        "compute_density_and_pressure",                    //
        foreach_particle(                                  //
            if_group_type(                                 //
                "fluid",                                   //
                rho[i] = 0.0,                              //
                foreach_neighbor(                          //
                    if_group_type(                         //
                        "fluid",                           //
                        rho[i] += common::f_f_density()    //
                        )                                  //
                    ),                                     //
                foreach_neighbor(                          //
                    if_group_type(                         //
                        "boundary",                        //
                        rho[i] += aiast12::f_b_density()   //
                        )                                  //
                    ),                                     //
                p[i] = common::f_pressure_state_equation() //
                )                                          //
            )                                              //
        ));

auto const g = gr_field("gravity", {0});
auto const a = vr_field("acceleration", {0});

registry.scheme_procedures(
    "sesph",                                                          //
    procedure(                                                        //
        "compute_acceleration",                                       //
        foreach_particle(                                             //
            if_group_type(                                            //
                "fluid",                                              //
                a[i] = 1.f * g,                                       //
                foreach_neighbor(                                     //
                    if_group_type(                                    //
                        "fluid",                                      //
                        a[i] -= common::f_f_pressure_acceleration(),  //
                        a[i] += common::f_f_viscosity_acceleration()  //
                        )                                             //
                    ),                                                //
                foreach_neighbor(                                     //
                    if_group_type(                                    //
                        "boundary",                                   //
                        a[i] -= aiast12::f_b_pressure_acceleration(), //
                        a[i] += aiast12::f_b_viscosity_acceleration() //
                        )                                             //
                    )                                                 //
                )                                                     //
            )                                                         //
        )                                                             //
);

PRTCL_GT_CLOSE_REGISTER_SCHEME_PROCEDURES
