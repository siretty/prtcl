
#include <prtcl/rt/basic_model_policy.hpp>
#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>
#include <prtcl/rt/grouped_uniform_grid.hpp>
#include <prtcl/rt/integral_grid.hpp>
#include <prtcl/rt/math/kernel/cubic_spline_kernel.hpp>
#include <prtcl/rt/math/kernel_math_policy_mixin.hpp>
#include <prtcl/rt/math/mixed_math_policy.hpp>
#include <prtcl/rt/neighborhood.hpp>
#include <prtcl/rt/save_vtk.hpp>
#include <prtcl/rt/vector_data_policy.hpp>

#include <prtcl/schemes/boundary.hpp>
#include <prtcl/schemes/sesph.hpp>
#include <prtcl/schemes/symplectic_euler.hpp>

#include <iostream>

#include <boost/filesystem.hpp>

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
  using math_policy = typename model_policy::math_policy;

  prtcl::rt::basic_model<model_policy> model;
  model.add_global<nd_dtype::real>("time_step");

  auto &f = model.add_group("f", "fluid");
  auto &b = model.add_group("b", "boundary");

  prtcl::schemes::boundary<model_policy> boundary;
  boundary.require(model);

  prtcl::schemes::sesph<model_policy> sesph;
  sesph.require(model);

  prtcl::schemes::symplectic_euler<model_policy> advect;
  advect.require(model);

  // -----

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
    f.get_uniform<nd_dtype::real>("mass")[0] =
        prtcl::core::constpow(h, N) * rho0;
  }

  size_t grid_size = 4; // 45;

  { // initialize fluid position and velocity
    auto const h = model.get_global<nd_dtype::real>("smoothing_scale")[0];

    prtcl::rt::integral_grid<N> x_grid;
    x_grid.extents.fill(grid_size);

    f.resize(x_grid.size());

    size_t i = 0;
    auto x = f.get_varying<nd_dtype::real, N>("position");
    auto v = f.get_varying<nd_dtype::real, N>("velocity");
    for (auto ix : x_grid) {
      x[i][0] = h * ix[0];
      x[i][1] = h * ix[1];
      x[i][2] = h * ix[2];
      v[i] = math_policy::constants::zeros<nd_dtype::real, N>();
      ++i;
    }
  }

  { // initialize boundary position
    auto const h = model.get_global<nd_dtype::real>("smoothing_scale")[0];

    std::array<prtcl::rt::integral_grid<N>, 3> grids = {
        prtcl::rt::integral_grid<N>{1, grid_size + 4, grid_size + 4},
        prtcl::rt::integral_grid<N>{grid_size + 4, 1, grid_size + 4},
        prtcl::rt::integral_grid<N>{grid_size + 4, grid_size + 4, 1}};

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

        x[i + 1][0] = x[i][0] + (grid_size + 3) * h;
        x[i + 1][1] = x[i][1] + (grid_size + 3) * h;
        x[i + 1][2] = x[i][2] + (grid_size + 3) * h;

        i += 2;
      }
    }
  }
  // }}}

  // -----

  prtcl::rt::neighbourhood<prtcl::rt::grouped_uniform_grid<model_policy>> nhood;

  nhood.load(model);
  nhood.update();

  boundary.load(model);
  sesph.load(model);

  boundary.compute_volume(nhood);

  namespace fs = boost::filesystem;
  auto output_dir = fs::current_path() / "output";

  size_t max_frame = 5;
  for (size_t frame = 0; frame <= max_frame; ++frame) {
    for (auto &g : model.groups()) {
      if ("fluid" != g.get_type())
        continue;
      auto file_name =
          std::string{g.get_name()} + std::to_string(frame) + ".vtk";
      auto file_path = output_dir / file_name;
      fs::ofstream file{file_path, file.trunc | file.out};
      std::cout << "START SAVE_VTK " << file_path << '\n';
      prtcl::rt::save_vtk(file, g);
      std::cout << "CLOSE SAVE_VTK " << file_path << '\n';
    }

    if (frame == max_frame)
      break;

    std::cout << "START OF FRAME #" << frame + 1 << '\n';
    sesph.compute_density_and_pressure(nhood);
    sesph.compute_acceleration(nhood);
    advect.advect_symplectic_euler(nhood);
    std::cout << "CLOSE OF FRAME #" << frame + 1 << '\n' << '\n';
  }
}
