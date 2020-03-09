#include <prtcl/rt/basic_application.hpp>

#include <prtcl/rt/math/aat13_math_policy_mixin.hpp>

#include <prtcl/schemes/aat13.hpp>
#include <prtcl/schemes/boundary.hpp>
#include <prtcl/schemes/gravity.hpp>
#include <prtcl/schemes/he14.hpp>
#include <prtcl/schemes/sesph.hpp>
#include <prtcl/schemes/symplectic_euler.hpp>
#include <prtcl/schemes/viscosity.hpp>

#include <iostream>
#include <string_view>

#define SURFACE_TENSION_AAT13 1
#define SURFACE_TENSION_He14 2

#define SURFACE_TENSION SURFACE_TENSION_AAT13

template <typename ModelPolicy_>
class sesph_application final
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
    require_all(
        model, boundary, sesph, gravity, viscosity, surface_tension, advect);
  }

  void on_load_schemes(model_type &model) override {
    load_all(
        model, boundary, sesph, gravity, viscosity, surface_tension, advect);
  }

  void
  on_prepare_simulation(model_type &model, neighborhood_type &nhood) override {
    boundary.compute_volume(nhood);

    auto g = model.template add_global<nd_dtype::real, N>("gravity");
    g[0] = c::template zeros<nd_dtype::real, N>();
    g[0][1] = -9.81;
  }

  void on_step(model_type &, neighborhood_type &nhood) override {
    sesph.compute_density_and_pressure(nhood);

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

    sesph.accumulate_acceleration(nhood);

    advect.advect_symplectic_euler(nhood);
  }

  prtcl::schemes::boundary<model_policy> boundary;
  prtcl::schemes::sesph<model_policy> sesph;
  prtcl::schemes::gravity<model_policy> gravity;
  prtcl::schemes::viscosity<model_policy> viscosity;
#if SURFACE_TENSION == SURFACE_TENSION_AAT13
  prtcl::schemes::aat13<model_policy> surface_tension;
#endif
#if SURFACE_TENSION == SURFACE_TENSION_He14
  prtcl::schemes::he14<model_policy> surface_tension;
#endif
  prtcl::schemes::symplectic_euler<model_policy> advect;
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

  sesph_application<model_policy> application;
  application.main(argc_, argv_);
}
