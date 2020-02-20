
#include <prtcl/rt/basic_model_policy.hpp>
#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>
#include <prtcl/rt/filesystem/getcwd.hpp>
#include <prtcl/rt/grouped_uniform_grid.hpp>
#include <prtcl/rt/integral_grid.hpp>
#include <prtcl/rt/math/kernel/cubic_spline_kernel.hpp>
#include <prtcl/rt/math/kernel_math_policy_mixin.hpp>
#include <prtcl/rt/math/mixed_math_policy.hpp>
#include <prtcl/rt/neighborhood.hpp>
#include <prtcl/rt/save_vtk.hpp>
#include <prtcl/rt/triangle_mesh.hpp>
#include <prtcl/rt/vector_data_policy.hpp>
#include <prtcl/rt/virtual_clock.hpp>

#include <prtcl/schemes/boundary.hpp>
#include <prtcl/schemes/sesph_v2.hpp>
#include <prtcl/schemes/symplectic_euler.hpp>

#include <fstream>
#include <iostream>
#include <random>

#include <cstdlib>

#include <unistd.h>

int main(int, char **) {
  using prtcl::core::nd_dtype;

  constexpr size_t N = 3;

  using model_policy = prtcl::rt::basic_model_policy<
      prtcl::rt::fib_type_policy,
      prtcl::rt::mixed_math_policy<
          prtcl::rt::eigen_math_policy,
          prtcl::rt::kernel_math_policy_mixin<prtcl::rt::cubic_spline_kernel>>::
          template policy,
      prtcl::rt::vector_data_policy, N>;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;

  using real = typename type_policy::real;

  prtcl::rt::basic_model<model_policy> model;

  auto &f = model.add_group("f", "fluid");
  auto &b = model.add_group("b", "boundary");

  prtcl::schemes::boundary<model_policy> boundary;
  boundary.require(model);

  prtcl::schemes::sesph<model_policy> sesph;
  sesph.require(model);

  prtcl::schemes::symplectic_euler<model_policy> advect;
  advect.require(model);

  // -----

  size_t grid_size = 16; // 45;

  // {{{
  { // initialize global and uniform fields
    auto const h = 0.025f;
    auto const rho0 = 1000.f;

    model.get_global<nd_dtype::real>("smoothing_scale")[0] = h;
    model.get_global<nd_dtype::real>("time_step")[0] = 0.00001f;

    model.get_global<nd_dtype::real, N>("gravity")[0] =
        math_policy::constants::zeros<nd_dtype::real, N>();
    model.get_global<nd_dtype::real, N>("gravity")[0][1] = -9.81f;

    f.get_uniform<nd_dtype::real>("rest_density")[0] = rho0;
    f.get_uniform<nd_dtype::real>("compressibility")[0] = 10'000'000;
    f.get_uniform<nd_dtype::real>("viscosity")[0] = 0.01f;
  }

  { // initialize fluid position and velocity
    auto const h = model.get_global<nd_dtype::real>("smoothing_scale")[0];
    auto const rho0 = f.get_uniform<nd_dtype::real>("rest_density")[0];

    std::mt19937 gen;
    std::uniform_real_distribution<typename type_policy::real> dis{-0.01f,
                                                                   0.01f};

    prtcl::rt::integral_grid<N> x_grid;
    x_grid.extents.fill(grid_size);
    x_grid.extents[1] *= 2;

    f.resize(x_grid.size());

    auto const base_m = prtcl::core::constpow(h, N) * rho0;

    auto x = f.get_varying<nd_dtype::real, N>("position");
    auto v = f.get_varying<nd_dtype::real, N>("velocity");
    auto m = f.get_varying<nd_dtype::real>("mass");

    size_t i = 0;
    for (auto ix : x_grid) {
      x[i][0] = h * ix[0] + dis(gen) * h;
      x[i][1] = h * ix[1] + dis(gen) * h;
      x[i][2] = h * ix[2] + dis(gen) * h;
      v[i] = math_policy::constants::zeros<nd_dtype::real, N>();
      m[i] = base_m + dis(gen) * base_m / 10;
      ++i;
    }
  }

  { // initialize boundary position
    auto const h = model.get_global<nd_dtype::real>("smoothing_scale")[0];

    grid_size *= 2;

    std::array<prtcl::rt::integral_grid<N>, 3> grids = {
        prtcl::rt::integral_grid<N>{1, grid_size + 4, grid_size + 4},
        prtcl::rt::integral_grid<N>{grid_size + 4, 1, grid_size + 4},
        prtcl::rt::integral_grid<N>{grid_size + 4, grid_size + 4, 1}};
    grids[0].extents[1] = (grids[0].extents[1] - 4) * 2 + 4;
    grids[2].extents[1] = (grids[2].extents[1] - 4) * 2 + 4;

    { // resize boundary group
      size_t count = 0;
      for (auto const &grid : grids)
        count += grid.size();
      b.resize(2 * count);
    }

    auto x = b.get_varying<nd_dtype::real, N>("position");

    size_t i = 0;
    for (size_t i_grid = 0; i_grid < grids.size(); ++i_grid) {
      auto const &grid = grids[i_grid];
      for (auto ix : grid) {
        x[i][0] = h * ix[0] - 2 * h;
        x[i][1] = h * ix[1] - 2 * h;
        x[i][2] = h * ix[2] - 2 * h;

        auto const j = i + 1;
        x[j][0] = (1 * grid_size + 1) * h - h * ix[0];
        x[j][1] = (2 * grid_size + 1) * h - h * ix[1];
        x[j][2] = (1 * grid_size + 1) * h - h * ix[2];

        i += 2;
      }
    }
  }
  // }}}

  // -----

  prtcl::rt::neighbourhood<prtcl::rt::grouped_uniform_grid<model_policy>> nhood;
  nhood.set_radius(math_policy::operations::kernel_support_radius(
      model.get_global<nd_dtype::real>("smoothing_scale")[0]));

  nhood.load(model);
  nhood.update();

  nhood.permute(model);

  boundary.load(model);
  sesph.load(model);
  advect.load(model);

  boundary.compute_volume(nhood);

  auto cwd = ::prtcl::rt::filesystem::getcwd();
  auto output_dir = cwd + "/" + "output";

  {
    std::ifstream obj_file{cwd + "/" + "share/models/unitcube.obj"};
    prtcl::rt::triangle_mesh<model_policy>::load_from_obj(obj_file);
  }

  for (auto &g : model.groups()) {
    if ("boundary" != g.get_type())
      continue;
    auto file_name = std::string{g.get_name()} + ".vtk";
    auto file_path = output_dir + "/" + file_name;
    std::ofstream file{file_path, file.trunc | file.out};
    std::cout << "START SAVE_VTK " << output_dir << '\n';
    prtcl::rt::save_vtk(file, g);
    std::cout << "CLOSE SAVE_VTK " << file_path << '\n';
  }

  size_t const fps = 30;
  size_t const max_frame = 60 * fps;

  prtcl::rt::virtual_clock<long double> clock;
  using duration = typename decltype(clock)::duration;

  auto const seconds_per_frame = (1.0L / fps);

  auto const max_cfl = 0.7L;
  auto const max_time_step = seconds_per_frame / 150;

  for (size_t frame = 0; frame <= max_frame; ++frame) {
    for (auto &g : model.groups()) {
      if ("fluid" != g.get_type())
        continue;
      auto file_name =
          std::string{g.get_name()} + "." + std::to_string(frame) + ".vtk";
      auto file_path = output_dir + "/" + file_name;
      std::ofstream file{file_path, file.trunc | file.out};
      std::cout << "START SAVE_VTK " << output_dir << '\n';
      prtcl::rt::save_vtk(file, g);
      std::cout << "CLOSE SAVE_VTK " << file_path << '\n' << '\n';
    }

    if (frame == max_frame)
      break;

    // permute the particles according to the neighborhood
    if (0 == frame % 4)
      nhood.permute(model);

    auto h = static_cast<long double>(
        model.get_global<nd_dtype::real>("smoothing_scale")[0]);

    auto frame_done = clock.now() + duration{seconds_per_frame};

    std::cout << "START OF FRAME #" << frame + 1 << '\n';
    while (clock.now() < frame_done) {
      // update the neighborhood
      nhood.update();

      // run all computational steps
      sesph.compute_density_and_pressure(nhood);
      sesph.compute_acceleration(nhood);
      advect.advect_symplectic_euler(nhood);

      // fetch the maximum speed of any fluid particle
      auto max_speed = static_cast<long double>(
          model.get_global<nd_dtype::real>("maximum_speed")[0]);

      auto dt = static_cast<long double>(
          model.get_global<nd_dtype::real>("time_step")[0]);

      // advance the simulation clock
      clock.advance(dt);

      // modify the timestep
      model.get_global<nd_dtype::real>("time_step")[0] =
          static_cast<real>(std::min(max_cfl * h / max_speed, max_time_step));
    }
    std::cout << "CLOSE OF FRAME #" << frame + 1 << '\n' << '\n';
  }
}
