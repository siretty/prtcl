#pragma once

#include <prtcl/data/group_base.hpp>
#include <prtcl/expr/call.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/expr/section.hpp>

#define PRTCL_SCHEME_SESPH_COMMON                                              \
  using namespace prtcl::expr_language;                                        \
  using namespace prtcl::expr_literals;                                        \
                                                                               \
  auto const i [[maybe_unused]] = prtcl::tag::group::active{};                 \
  auto const j [[maybe_unused]] = prtcl::tag::group::passive{};                \
                                                                               \
  auto const h = "smoothing_scale"_gs;                                         \
  auto const g = "gravity"_gv;                                                 \
  auto const dt = "time_step"_gs;                                              \
                                                                               \
  auto const x = "position"_vv;                                                \
  auto const v = "velocity"_vv;                                                \
  auto const a = "acceleration"_vv;                                            \
                                                                               \
  auto const m = "mass"_us;                                                    \
  auto const rho0 = "rest_density"_us;                                         \
  auto const kappa = "compressibility"_us;                                     \
  auto const nu = "viscosity"_us;                                              \
                                                                               \
  auto const rho = "density"_vs;                                               \
  auto const p = "pressure"_vs;                                                \
  auto const V = "volume"_vs;                                                  \
                                                                               \
  auto select_fluid [[maybe_unused]] = [](prtcl::data::group_base const &g) {  \
    return g.has_flag("fluid");                                                \
  };                                                                           \
  auto select_boundary [[maybe_unused]] =                                      \
      [](prtcl::data::group_base const &g) { return g.has_flag("boundary"); };

namespace prtcl::scheme {

inline auto sesph_boundary_volume() {
  PRTCL_SCHEME_SESPH_COMMON

  return make_named_section(                          //
      "SESPH Boundary Volume",                        //
      foreach_particle(                               //
          only(select_boundary)(                      //
              V[i] = 0,                               //
              foreach_neighbour(                      //
                  only(select_boundary)(              //
                      V[i] += kernel(x[i] - x[j], h)) //
                  ),                                  //
              V[i] = 1 / V[i]                         //
              )                                       //
          )                                           //
  );
}

inline auto sesph_density_pressure() {
  PRTCL_SCHEME_SESPH_COMMON

  return make_named_section(                                            //
      "SESPH Density and Pressure",                                     //
      foreach_particle(                                                 //
          only(select_fluid)(                                           //
              rho[i] = 0,                                               //
              foreach_neighbour(                                        //
                  only(select_fluid)(                                   //
                      rho[i] += m[j] * kernel(x[i] - x[j], h)           //
                      ),                                                //
                  only(select_boundary)(                                //
                      rho[i] += rho0[i] * V[j] * kernel(x[i] - x[j], h) //
                      )                                                 //
                  ),                                                    //
              p[i] = (1 / kappa[i]) * max(0, rho[i] / rho0[i] - 1)      //
              )                                                         //
          )                                                             //
  );
}

inline auto sesph_acceleration() {
  PRTCL_SCHEME_SESPH_COMMON

  auto ff_viscosity = //
      nu[j] * (m[j] / rho[j]) *
      (dot(v[i] - v[j], x[i] - x[j]) /
       (norm_squared(x[i] - x[j]) + 0.01 * h * h)) *
      kernel_gradient(x[i] - x[j], h);

  auto ff_pressure = //
      m[j] * (p[i] / (rho[i] * rho[i]) + p[j] / (rho[j] * rho[j])) *
      kernel_gradient(x[i] - x[j], h);

  auto fb_viscosity = //
      nu[j] * V[j] / (rho0[i] * rho[i]) *
      (dot(v[i], x[i] - x[j]) / (norm_squared(x[i] - x[j]) + 0.01 * h * h)) *
      kernel_gradient(x[i] - x[j], h);

  auto fb_pressure = //
      2 * 0.7 * (rho0[i] * V[j]) * (p[i] / (rho[i] * rho[i])) *
      kernel_gradient(x[i] - x[j], h);

  return make_named_section(                //
      "SESPH Acceleration",                 //
      foreach_particle(                     //
          only(select_fluid)(               //
              a[i] = g,                     //
              foreach_neighbour(            //
                  only(select_fluid)(       //
                      a[i] += ff_viscosity, //
                      a[i] -= ff_pressure   //
                      ),                    //
                  only(select_boundary)(    //
                      a[i] += fb_viscosity, //
                      a[i] -= fb_pressure   //
                      )                     //
                  )                         //
              )                             //
          )                                 //
  );
}

inline auto sesph_symplectic_euler() {
  PRTCL_SCHEME_SESPH_COMMON

  return make_named_section(     //
      "SESPH Symplectic Euler",  //
      foreach_particle(          //
          only(select_fluid)(    //
              v[i] += dt * a[i], //
              x[i] += dt * v[i]  //
              )                  //
          )                      //
  );
}

} // namespace prtcl::scheme

#undef PRTCL_SCHEME_SESPH_COMMON