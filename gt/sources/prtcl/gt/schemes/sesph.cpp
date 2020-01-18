#include <prtcl/gt/dsl.hpp>
#include <prtcl/gt/schemes_registry.hpp>

PRTCL_GT_START_REGISTER_SCHEME_PROCEDURES(sesph)

using namespace prtcl::gt::dsl::language;
using namespace prtcl::gt::dsl::generic_indices;
using namespace prtcl::gt::dsl::kernel_shorthand;

auto const x = vr_field("position", {0});
auto const m = ur_field("mass", {});

auto const f_f_density = m[j] * W(x[i] - x[j]);

auto const rho0 = ur_field("rest_density", {});
auto const V = vr_field("volume", {});

auto const f_b_density = V[j] * rho0[i] * W(x[i] - x[j]);

auto const rho = vr_field("density", {});
auto const p = vr_field("pressure", {});
auto const k = ur_field("compressibility", {});

registry.scheme_procedures(
    "sesph",                                                   //
    procedure(                                                 //
        "compute_density_and_pressure",                        //
        foreach_particle(                                      //
            if_group_type(                                     //
                "fluid",                                       //
                rho[i] = 0.0,                                  //
                foreach_neighbor(                              //
                    if_group_type(                             //
                        "fluid",                               //
                        rho[i] += f_f_density                  //
                        )                                      //
                    ),                                         //
                foreach_neighbor(                              //
                    if_group_type(                             //
                        "boundary",                            //
                        rho[i] += f_b_density                  //
                        )                                      //
                    ),                                         //
                p[i] = k[i] * max(0.0, rho[i] / rho0[i] - 1.0) //
                )                                              //
            )                                                  //
        ));

auto const a = vr_field("acceleration", {0});

auto const f_f_pressure_acceleration = //
    m[j] * (p[i] / (rho[i] * rho[i]) + p[j] / (rho[j] * rho[j])) *
    dW(x[i] - x[j]);

auto const f_b_pressure_acceleration = //
    0.7 * V[j] * rho0[i] * (2 * p[i] / (rho[i] * rho[i])) * dW(x[i] - x[j]);

auto const v = vr_field("velocity", {0});
auto const h = gr_field("smoothing_scale", {});
auto const nu = ur_field("viscosity", {});

auto const f_f_viscosity_acceleration = //
    (nu[i] * (m[j] / rho[j]) * dot(v[i] - v[j], x[i] - x[j]) /
     (norm_squared(x[i] - x[j]) + 0.01 * h * h)) *
    dW(x[i] - x[j]);

auto const f_b_viscosity_acceleration = //
    (nu[i] * V[j] * dot(v[i], x[i] - x[j]) /
     (norm_squared(x[i] - x[j]) + 0.01 * h * h)) *
    dW(x[i] - x[j]);

auto const g = gr_field("gravity", {0});

registry.scheme_procedures(
    "sesph",                                               //
    procedure(                                             //
        "compute_acceleration",                            //
        foreach_particle(                                  //
            if_group_type(                                 //
                "fluid",                                   //
                a[i] = 1.f * g,                            //
                foreach_neighbor(                          //
                    if_group_type(                         //
                        "fluid",                           //
                        a[i] -= f_f_pressure_acceleration, //
                        a[i] += f_f_viscosity_acceleration //
                        )                                  //
                    ),                                     //
                foreach_neighbor(                          //
                    if_group_type(                         //
                        "boundary",                        //
                        a[i] -= f_b_pressure_acceleration, //
                        a[i] += f_b_viscosity_acceleration //
                        )                                  //
                    )                                      //
                )                                          //
            )                                              //
        )                                                  //
);

PRTCL_GT_CLOSE_REGISTER_SCHEME_PROCEDURES
