#include <prtcl/core/log/logger.hpp>

#include <prtcl/rt/basic_application.hpp>
#include <prtcl/rt/math/aat13_math_policy_mixin.hpp>

#include <prtcl/schemes/aat13.hpp>
#include <prtcl/schemes/aiast12.hpp>
#include <prtcl/schemes/he14.hpp>
#include <prtcl/schemes/wkbb18.hpp>

#include <prtcl/schemes/iisph.hpp>

#include <prtcl/schemes/density.hpp>
#include <prtcl/schemes/gravity.hpp>
#include <prtcl/schemes/viscosity.hpp>

#include <prtcl/schemes/symplectic_euler.hpp>

#include <iostream>
#include <string_view>

#include <cfenv>

#define SURFACE_TENSION_AAT13 1
#define SURFACE_TENSION_He14 2

#define SURFACE_TENSION SURFACE_TENSION_AAT13

template <typename ModelPolicy_>
class iisph_wkbb18_application final
    : public prtcl::rt::basic_application<ModelPolicy_> {
  using base_type = prtcl::rt::basic_application<ModelPolicy_>;

public:
  using model_policy = typename base_type::model_policy;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;

  using model_type = typename base_type::model_type;
  using neighborhood_type = typename base_type::neighborhood_type;

  using dtype = prtcl::core::dtype;

  using real = typename type_policy::real;
  using o = typename math_policy::operations;

  static constexpr size_t N = model_policy::dimensionality;

public:
  void on_require_schemes(model_type &model) override {
    require_all(
        model, advect, boundary, density, iisph, gravity, viscosity,
        implicit_viscosity);
#if SURFACE_TENSION != 0
    surface_tension.require(model);
#endif
  }

  void on_load_schemes(model_type &model) override {
    load_all(
        model, advect, boundary, density, iisph, gravity, viscosity,
        implicit_viscosity);
#if SURFACE_TENSION != 0
    surface_tension.load(model);
#endif
  }

  void
  on_prepare_simulation(model_type &model, neighborhood_type &nhood) override {
    boundary.compute_volume(nhood);

    auto g = model.template add_global<dtype::real, N>("gravity");
    g[0] = o::template zeros<dtype::real, N>();
    g[0][1] = static_cast<real>(-9.81);

    //::feenableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID);
  }

  void on_prepare_step(model_type &, neighborhood_type &) override {
    // nothing to do
  }

  void on_step(model_type &model, neighborhood_type &nhood) override {
    namespace log = prtcl::core::log;

    density.compute_density(nhood);

    gravity.initialize_acceleration(nhood);

#if SURFACE_TENSION == SURFACE_TENSION_AAT13
    // AAT13: Surface Tension
    surface_tension.compute_particle_normal(nhood);
#endif
#if SURFACE_TENSION == SURFACE_TENSION_He14
    // He14: Surface Tension
    surface_tension.compute_color_field(nhood);
    surface_tension.compute_color_field_gradient(nhood);
#endif
#if SURFACE_TENSION != 0
    // AAT13 + He14: Surface Tension
    surface_tension.accumulate_acceleration(nhood);
#endif

    // predict the velocity at the next timestep (explicit)
    advect.integrate_velocity_with_hard_fade(nhood);

    { // pressure solver
      PRTCL_RT_LOG_TRACE_SCOPED("pressure solver");

      auto aprde = model.template get_global<dtype::real>("iisph_aprde");
      auto nprde = model.template get_global<dtype::integer>("iisph_nprde");

      // reset the particle counter
      nprde[0] = 0;
      iisph.setup(nhood);

      constexpr int min_solver_iterations = 3;
      constexpr int max_solver_iterations = 2000;
      constexpr auto const max_aprde = static_cast<real>(0.001);

      real cur_aprde = 0;

      int pressure_iteration = 0;
      for (;
           pressure_iteration < min_solver_iterations or cur_aprde > max_aprde;
           ++pressure_iteration) {
        // break the loop if we reached the maximum number of iterations
        if (pressure_iteration >= max_solver_iterations)
          break;

        // break the loop if there are no particles
        if (nprde[0] == 0)
          break;

        // reset the accum. positive relative density error
        aprde[0] = 0;

        iisph.iteration_pressure_acceleration(nhood);
        iisph.iteration_pressure(nhood);

        // compute the average positive relative density error
        cur_aprde = aprde[0] / static_cast<real>(nprde[0]);
      }

      // prtcl::core::log::debug(
      //    "app", "iisph", "last aprde = ", aprde[0],
      //    " cur_aprde = ", cur_aprde);
      prtcl::core::log::debug(
          "app", "iisph", "no. iterations ", pressure_iteration);
    }

    // predict the velocity at the next timestep (explicit + pressure)
    advect.integrate_velocity_with_hard_fade(nhood);

    { // viscosity solver
      PRTCL_RT_LOG_TRACE_SCOPED("viscosity solver");

      implicit_viscosity.compute_diagonal(nhood);
      implicit_viscosity.accumulate_acceleration(nhood);

      // prtcl::core::log::debug("app", "wkbb18", "no. iterations ",
      // iterations);
    }

    // integrate the velocity (explicit + pressure + viscosity)
    advect.integrate_velocity_with_hard_fade(nhood);

    // integrate the particle position
    advect.integrate_position(nhood);
  }

  prtcl::schemes::density<model_policy> density;
  prtcl::schemes::symplectic_euler<model_policy> advect;
  prtcl::schemes::aiast12<model_policy> boundary;
  prtcl::schemes::iisph<model_policy> iisph;
  prtcl::schemes::gravity<model_policy> gravity;
  prtcl::schemes::viscosity<model_policy> viscosity;
  prtcl::schemes::wkbb18<model_policy> implicit_viscosity;
#if SURFACE_TENSION == SURFACE_TENSION_AAT13
  prtcl::schemes::aat13<model_policy> surface_tension;
#endif
#if SURFACE_TENSION == SURFACE_TENSION_He14
  prtcl::schemes::he14<model_policy> surface_tension;
#endif
};

constexpr size_t N = 3;

using model_policy = prtcl::rt::basic_model_policy<
    prtcl::rt::fib_type_policy,
    prtcl::rt::mixed_math_policy<
        prtcl::rt::eigen_math_policy,
        // SPH kernel function
        prtcl::rt::kernel<prtcl::rt::cubic_spline_kernel>,
        // [AAT13] functions
        prtcl::rt::aat13_math_policy_mixin //
        >::template policy,
    prtcl::rt::vector_data_policy, N>;

int main(int argc_, char **argv_) {
  std::cerr << "PRESS [ENTER] TO START";
  std::getchar();

  iisph_wkbb18_application<model_policy> application;
  return application.main(argc_, argv_);
}
