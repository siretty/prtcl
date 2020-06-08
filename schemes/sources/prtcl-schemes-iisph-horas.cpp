#include <prtcl/rt/basic_application.hpp>
#include <prtcl/rt/camera.hpp>
#include <prtcl/rt/math/aat13_math_policy_mixin.hpp>

#include <prtcl/core/log/logger.hpp>

#include <prtcl/schemes/aat13.hpp>
#include <prtcl/schemes/aiast12.hpp>
#include <prtcl/schemes/density.hpp>
#include <prtcl/schemes/gravity.hpp>
#include <prtcl/schemes/he14.hpp>
#include <prtcl/schemes/horas.hpp>
#include <prtcl/schemes/iisph.hpp>
#include <prtcl/schemes/symplectic_euler.hpp>
#include <prtcl/schemes/viscosity.hpp>

#include <iostream>
#include <string_view>

#define SURFACE_TENSION_AAT13 1
#define SURFACE_TENSION_He14 2

#define SURFACE_TENSION SURFACE_TENSION_AAT13

namespace log = prtcl::core::log;

template <typename ModelPolicy_>
class iisph_horas_application final
    : public prtcl::rt::basic_application<ModelPolicy_> {
  using base_type = prtcl::rt::basic_application<ModelPolicy_>;

public:
  using model_policy = typename base_type::model_policy;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;

  using model_type = typename base_type::model_type;
  using neighborhood_type = typename base_type::neighborhood_type;

  using dtype = prtcl::core::dtype;

  static constexpr size_t N = model_policy::dimensionality;

  using real = typename type_policy::real;
  using rvec = typename math_policy::template ndtype_t<dtype::real, N>;

  using o = typename math_policy::operations;

public:
  void on_require_schemes(model_type &model) override {
    // add the group that will contain the horasons
    model.add_group("camera", "horason");

    require_all(
        model, advect, boundary, density, iisph, gravity, viscosity, horas);
#if SURFACE_TENSION != 0
    surface_tension.require(model);
#endif
  }

  void on_load_schemes(model_type &model) override {
    load_all(
        model, advect, boundary, density, iisph, gravity, viscosity, horas);
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

    { // setting up the fluid group for rendering
      auto &group = model.get_group("f");
      group.add_tag("visible");
    }

    { // setup the camera and horasons
      camera.camera.origin = o::template zeros<dtype::real, N>();
      camera.camera.origin[0] = -3;

      camera.camera.principal = o::template zeros<dtype::real, N>();
      camera.camera.principal[0] = 1;

      camera.camera.up = o::template zeros<dtype::real, N>();
      camera.camera.up[1] = static_cast<real>(1);

      camera.camera.focal_length = 1;

      camera.sensor.width = 640;
      camera.sensor.height = 480;

      auto &group = model.get_group("camera");
      group.add_tag("cannot_be_neighbor");

      group.resize(camera.sensor.width * camera.sensor.height);
      group.dirty(true);

      auto v_x = group.template get_varying<dtype::real, N>("position");
      auto v_x0 =
          group.template get_varying<dtype::real, N>("initial_position");
      auto v_d = group.template get_varying<dtype::real, N>("direction");

      camera.cast([&sensor = camera.sensor, &v_x, &v_x0,
                   &v_d](auto ix, auto iy, rvec x0, rvec d) {
        auto index = iy * sensor.width + ix;
        v_x[index] = x0;
        v_x0[index] = x0;
        v_d[index] = d;

        // prtcl::core::log::debug(
        //    "iisph-horas", "setup_horason", "ix=", ix, " iy=", iy,
        //    " ii=", index);
      });
    }
  }

  void on_prepare_step(model_type &, neighborhood_type &) override {
    // nothing to do
  }

  void on_step(model_type &model, neighborhood_type &nhood) override {
    namespace log = prtcl::core::log;

    density.compute_density(nhood);

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
#if SURFACE_TENSION != 0
    // AAT13 + He14: Surface Tension
    surface_tension.accumulate_acceleration(nhood);
#endif

    // predict the velocity at the next timestep (explicit)
    advect.integrate_velocity_with_hard_fade(nhood);

    { // pressure solver
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
        if (nprde[0] == 0) {
          log::debug(
              "app", "iisph", "pressure iteration canceled, no particles");
          break;
        }

        // reset the accum. positive relative density error
        aprde[0] = 0;

        iisph.iteration_pressure_acceleration(nhood);
        iisph.iteration_pressure(nhood);

        // compute the average positive relative density error
        cur_aprde = aprde[0] / static_cast<real>(nprde[0]);
      }

      log::debug(
          "app", "iisph", "last aprde = ", aprde[0],
          " cur_aprde = ", cur_aprde);
      log::debug("app", "iisph", "no. iterations ", pressure_iteration);
    }

    // predict the velocity at the next timestep (explicit + pressure)
    advect.integrate_velocity_with_hard_fade(nhood);

    // integrate the particle position
    advect.integrate_position(nhood);
  }

  void on_frame_done(model_type &model, neighborhood_type &nhood) override {
    namespace log = prtcl::core::log;

    static int frame = 1;

    horas.reset(nhood);

    for (size_t step_i = 0; step_i < 300; ++step_i) {
      // TESTING: one rendering step
      log::debug("app", "horas", "step");
      horas.step(nhood);
      log::debug("app", "horas", "done");
    }

    auto &group = model.get_group("camera");
    //auto v_phi = group.template get_varying<dtype::real>("implicit_function");
    //auto v_t = group.template get_varying<dtype::real>("parameter");
    //for (size_t i = 0; i < v_phi.size(); ++i) {
    //  log::debug("app", "horas", "i=", i, " t=", v_t[i], " phi=", v_phi[i]);
    //}

    auto cwd = ::prtcl::rt::filesystem::getcwd();
    auto output_dir = cwd + "/" + "output";
    save_group(output_dir, group, frame++);
  }

  prtcl::schemes::density<model_policy> density;
  prtcl::schemes::symplectic_euler<model_policy> advect;
  prtcl::schemes::aiast12<model_policy> boundary;
  prtcl::schemes::iisph<model_policy> iisph;
  prtcl::schemes::gravity<model_policy> gravity;
  prtcl::schemes::viscosity<model_policy> viscosity;
#if SURFACE_TENSION == SURFACE_TENSION_AAT13
  prtcl::schemes::aat13<model_policy> surface_tension;
#endif
#if SURFACE_TENSION == SURFACE_TENSION_He14
  prtcl::schemes::he14<model_policy> surface_tension;
#endif

  prtcl::schemes::horas<model_policy> horas;
  prtcl::rt::pinhole_camera<model_policy> camera;
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

  iisph_horas_application<model_policy> application;
  application.main(argc_, argv_);
}
