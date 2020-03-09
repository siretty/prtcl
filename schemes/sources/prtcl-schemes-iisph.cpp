#include <prtcl/rt/basic_application.hpp>

#include <prtcl/rt/log/logger.hpp>
#include <prtcl/rt/math/aat13_math_policy_mixin.hpp>

#include <prtcl/schemes/aat13.hpp>
#include <prtcl/schemes/boundary.hpp>
#include <prtcl/schemes/gravity.hpp>
#include <prtcl/schemes/he14.hpp>
#include <prtcl/schemes/iisph.hpp>
#include <prtcl/schemes/symplectic_euler.hpp>
#include <prtcl/schemes/viscosity.hpp>
#include <prtcl/schemes/pt16.hpp>

#include <iostream>
#include <string_view>

#define SURFACE_TENSION_AAT13 1
#define SURFACE_TENSION_He14 2

#define SURFACE_TENSION SURFACE_TENSION_AAT13

template <typename ModelPolicy_>
class iisph_application final
    : public prtcl::rt::basic_application<ModelPolicy_> {
  using base_type = prtcl::rt::basic_application<ModelPolicy_>;

public:
  using model_policy = typename base_type::model_policy;
  using math_policy = typename model_policy::math_policy;

  using model_type = typename base_type::model_type;
  using neighborhood_type = typename base_type::neighborhood_type;

  using nd_dtype = prtcl::core::nd_dtype;

  using c = typename math_policy::constants;

  static constexpr size_t N = model_policy::dimensionality;

public:
  void on_require_schemes(model_type &model) override {
    require_all(model, boundary, iisph, gravity, viscosity, surface_tension, implicit_viscosity);
  }

  void on_load_schemes(model_type &model) override {
    load_all(model, boundary, iisph, gravity, viscosity, surface_tension, implicit_viscosity);
  }

  void
  on_prepare_simulation(model_type &model, neighborhood_type &nhood) override {
    boundary.compute_volume(nhood);

    auto g = model.template add_global<nd_dtype::real, N>("gravity");
    g[0] = c::template zeros<nd_dtype::real, N>();
    g[0][1] = -9.81;

    model.template get_global<nd_dtype::real>("maximum_time_step")[0] = 0.002;
  }

  void on_prepare_step(model_type &model, neighborhood_type &) override {
    //model.template get_global<nd_dtype::real>("fade_duration")[0] = 0;
  }

  void on_step(model_type &model, neighborhood_type &nhood) override {
    iisph.compute_density(nhood);

    gravity.initialize_acceleration(nhood);
    viscosity.accumulate_acceleration(nhood);

#if SURFACE_TENSION == SURFACE_TENSION_AAT13
    // AAT13: Surface Tension
    surface_tension.compute_particle_normal(nhood);
#endif
#if SURFACE_TENSION == SURFACE_TENSION_He14
    // He14: Surface Tension
    surface_tension.compute_color_field(nhood);
    surface_tension.compute_color_field_gradient(nhood);
#endif
    // AAT13 + He14: Surface Tension
    surface_tension.accumulate_acceleration(nhood);

    auto aprde = model.template get_global<nd_dtype::real>("iisph_aprde");
    auto nprde = model.template get_global<nd_dtype::integer>("iisph_nprde");

    iisph.predict_velocity(nhood);
    iisph.setup_a(nhood);

    // reset the particle counter
    nprde[0] = 0;
    iisph.setup_b(nhood);

    constexpr int min_solver_iterations = 3;
    constexpr int max_solver_iterations = 2000;
    constexpr typename model_policy::type_policy::real max_aprde = 0.001;

    typename model_policy::type_policy::real cur_aprde = 0;

    int solver_iteration = 0;
    for (; solver_iteration < min_solver_iterations or cur_aprde > max_aprde;
         ++solver_iteration) {
      // break the loop if we reached the maximum number of iterations
      if (solver_iteration >= max_solver_iterations)
        break;

      // break the loop if there are no particles
      if (nprde[0] == 0)
        break;

      // reset the accum. positive relative density error
      aprde[0] = 0;

      iisph.iteration_pressure_acceleration(nhood);
      iisph.iteration_pressure(nhood);

      // compute the average positive relative density error
      cur_aprde = aprde[0] / nprde[0];
      prtcl::rt::log::debug(
          "app", "iisph", "aprde = ", aprde[0], " cur_aprde = ", cur_aprde);
    }

    prtcl::rt::log::debug("app", "iisph", "no. iterations ", solver_iteration);

    iisph.advect(nhood);

    implicit_viscosity.compute_velocity_gradient_and_vorticity(nhood);
  }

  prtcl::schemes::boundary<model_policy> boundary;
  prtcl::schemes::iisph<model_policy> iisph;
  prtcl::schemes::gravity<model_policy> gravity;
  prtcl::schemes::viscosity<model_policy> viscosity;
#if SURFACE_TENSION == SURFACE_TENSION_AAT13
  prtcl::schemes::aat13<model_policy> surface_tension;
#endif
#if SURFACE_TENSION == SURFACE_TENSION_He14
  prtcl::schemes::he14<model_policy> surface_tension;
#endif
  prtcl::schemes::pt16<model_policy> implicit_viscosity;
};

constexpr size_t N = 3;

using model_policy = prtcl::rt::basic_model_policy<
    prtcl::rt::fib_type_policy,
    prtcl::rt::mixed_math_policy<
        prtcl::rt::eigen_math_policy,
        // SPH kernel function
        prtcl::rt::kernel_math_policy_mixin<prtcl::rt::cubic_spline_kernel>,
        // [AAT13] functions
        prtcl::rt::aat13_math_policy_mixin //
        >::template policy,
    prtcl::rt::vector_data_policy, N>;

int main(int argc_, char **argv_) {
  std::cerr << "PRESS [ENTER] TO START";
  std::getchar();

  iisph_application<model_policy> application;
  application.main(argc_, argv_);
}
