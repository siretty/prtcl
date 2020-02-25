
#include <prtcl/rt/basic_model_policy.hpp>
#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/cli/load_groups.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>
#include <prtcl/rt/filesystem/getcwd.hpp>
#include <prtcl/rt/geometry/axis_aligned_box.hpp>
#include <prtcl/rt/geometry/triangle_mesh.hpp>
#include <prtcl/rt/grouped_uniform_grid.hpp>
#include <prtcl/rt/integral_grid.hpp>
#include <prtcl/rt/math/kernel/cubic_spline_kernel.hpp>
#include <prtcl/rt/math/kernel_math_policy_mixin.hpp>
#include <prtcl/rt/math/mixed_math_policy.hpp>
#include <prtcl/rt/neighborhood.hpp>
#include <prtcl/rt/save_vtk.hpp>
#include <prtcl/rt/vector_data_policy.hpp>
#include <prtcl/rt/virtual_clock.hpp>

#include <prtcl/rt/cli/command_line_interface.hpp>

#include <prtcl/rt/sample_surface.hpp>
#include <prtcl/rt/sample_volume.hpp>

#include <prtcl/schemes/boundary.hpp>
#include <prtcl/schemes/sesph.hpp>
#include <prtcl/schemes/symplectic_euler.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>

#include <cstdlib>

#include <unistd.h>

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

using c = typename math_policy::constants;
using o = typename math_policy::operations;

using triangle_mesh_type = prtcl::rt::triangle_mesh<model_policy>;

using real = typename type_policy::real;
using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, N>;

int main(int argc_, char **argv_) {
  prtcl::rt::command_line_interface cli{argc_, argv_};

  boost::property_tree::write_json(std::cout, cli.name_value());

  triangle_mesh_type boundary_mesh;
  { // load the boundary geometry
    auto path = cli.value_with_name_or<std::string>(
        "boundary.geometry", "share/models/unitcube.obj");
    // open the geometry file for reading
    std::ifstream file{path, std::ios::in};
    // read the mesh from the file
    boundary_mesh = triangle_mesh_type::load_from_obj(file);
    // close the file
    file.close();
  }

  prtcl::rt::basic_model<model_policy> model;

  { // global default parameters
    auto const h = cli.value_with_name_or<real>(
        "parameter.smoothing_scale", static_cast<real>(0.025));

    model.add_global<nd_dtype::real>("smoothing_scale")[0] = h;
    model.add_global<nd_dtype::real>("time_step")[0] = 0.00001f;

    model.add_global<nd_dtype::real, N>("gravity")[0] =
        math_policy::constants::zeros<nd_dtype::real, N>();
    model.get_global<nd_dtype::real, N>("gravity")[0][1] = -9.81f;
  }

  prtcl::rt::load_model_groups_from_cli(cli, model);

  auto &f = model.add_group("f", "fluid");
  auto &b = model.add_group("b", "boundary");

  prtcl::schemes::boundary<model_policy> boundary;
  boundary.require(model);

  prtcl::schemes::sesph<model_policy> sesph;
  sesph.require(model);

  prtcl::schemes::symplectic_euler<model_policy> advect;
  advect.require(model);

  { // initialize uniform fields
    auto const rho0 = cli.value_with_name_or<real>(
        "parameter.rest_density", static_cast<real>(1000));

    f.get_uniform<nd_dtype::real>("rest_density")[0] = rho0;
    f.get_uniform<nd_dtype::real>("compressibility")[0] = 10'000'000;
    f.get_uniform<nd_dtype::real>("viscosity")[0] = 0.01f;
  }

  { // initialize uniform fields
    b.get_uniform<nd_dtype::real>("viscosity")[0] =
        10 * f.get_uniform<nd_dtype::real>("viscosity")[0];
  }

  // -----

  { // perturb fluid position and initialize velocity and mass
    // {{{
    using c = typename math_policy::constants;

    auto const h = model.get_global<nd_dtype::real>("smoothing_scale")[0];
    auto const rho0 = f.get_uniform<nd_dtype::real>("rest_density")[0];

    auto const base_m = prtcl::core::constpow(h, N) * rho0;

    std::mt19937 gen;
    std::uniform_real_distribution<typename type_policy::real> dis{-0.01f,
                                                                   0.01f};

    auto x = f.get_varying<nd_dtype::real, N>("position");
    auto v = f.get_varying<nd_dtype::real, N>("velocity");
    auto m = f.get_varying<nd_dtype::real>("mass");

    for (size_t i = 0; i < f.size(); ++i) {
      x[i] += h * rvec{dis(gen), dis(gen), dis(gen)};
      v[i] = c::zeros<nd_dtype::real, N>();
      m[i] = base_m + dis(gen) * base_m / 10;
    }
    // }}}
  }

  rvec b_lo = c::most_positive<nd_dtype::real, N>(),
       b_hi = c::most_negative<nd_dtype::real, N>();

  { // compute boundary aabb
    // {{{
    auto x_b = b.template get_varying<nd_dtype::real, N>("position");
    for (size_t i = 0; i < b.size(); ++i) {
      for (int n = 0; n < N; ++n) {
        b_lo[n] = std::min(b_lo[n], x_b[i][n]);
        b_hi[n] = std::max(b_hi[n], x_b[i][n]);
      }
    }
    // }}}
  }

  auto position_in_domain = [lo = rvec{b_lo - 0.1 * (b_hi - b_lo)},
                             hi = rvec{b_hi + 0.1 * (b_hi - b_lo)}] //
      (auto const &x) {
        return (lo.array() <= x.array()).all() and
               (x.array() <= hi.array()).all();
      };

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

  std::vector<size_t> remove_idxs, remove_perm;
  std::vector<rvec> spawned_positions, spawned_velocities;

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

    // remove particles by permuting them to the end and resizing the storage
    if (0 == frame % 4) {
      // {{{ implementation
      for (size_t group_index = 0; group_index < model.groups().size();
           ++group_index) {

        auto &group = model.get_group(group_index);
        if (group.get_type() != "fluid")
          continue;

        // find all particles in group that are out-of-bounds
        remove_idxs.clear();

        auto x = group.get_varying<nd_dtype::real, N>("position");
        for (size_t i = 0; i < x.size(); ++i)
          if (not position_in_domain(x[i]))
            remove_idxs.push_back(i);

        group.erase(remove_idxs);
      }

      // reload the neighborhood structure after removing particles
      nhood.load(model);
      // }}}
    }

    auto h = static_cast<long double>(
        model.get_global<nd_dtype::real>("smoothing_scale")[0]);

    auto frame_done = clock.now() + duration{seconds_per_frame};

    std::cout << "START OF FRAME #" << frame + 1 << '\n';
    while (clock.now() < frame_done) {
      // update the neighborhood
      nhood.update();

      bool spawned = false;

      // spawn new particles from each groups sources
      for (auto &group : model.groups()) {
        // {{{ implementation
        if (group.get_type() != "fluid")
          continue;

        spawned_positions.clear();
        spawned_velocities.clear();

        for (auto &source : group.sources()) {
          rvec const v_init = source.get_initial_velocity()[0];

          auto x = source.get_spawn_positions();
          for (size_t i = 0; i < source.size(); ++i) {
            real min_distance = c::positive_infinity<nd_dtype::real>();
            nhood.neighbors(
                x[i], [&model, &min_distance, &x, i](size_t g, size_t j) {
                  auto &g_n = model.get_group(g);
                  auto x_n = g_n.get_varying<nd_dtype::real, N>("position");
                  min_distance = std::min(min_distance, o::norm(x[i] - x_n[j]));
                });

            if (min_distance > static_cast<real>(1.1L * h)) {
              spawned_positions.emplace_back(x[i]);
              spawned_velocities.emplace_back(v_init);
            }
          }
        }

        if (spawned_positions.size() == 0)
          continue;

        spawned = true;

        auto indices = group.create(spawned_positions.size());

        auto rho0 = group.get_uniform<nd_dtype::real>("rest_density")[0];
        auto x = group.get_varying<nd_dtype::real, N>("position");
        auto v = group.get_varying<nd_dtype::real, N>("velocity");
        auto m = group.get_varying<nd_dtype::real>("mass");

        for (size_t i = 0; i < spawned_positions.size(); ++i) {
          x[indices[i]] = spawned_positions[i];
          v[indices[i]] = spawned_velocities[i];
          m[indices[i]] = static_cast<real>(prtcl::core::constpow(h, N)) * rho0;
        }

        // std::cerr << "spawned " << spawned_positions.size()
        //          << " particles in group " << group.get_name() << std::endl;

        // }}}
      }

      // if new particles were spawned, reload the neighborhood and the schemes
      if (spawned) {
        // std::cerr << "reloading neighborhood and schemes" << std::endl;

        nhood.load(model);
        nhood.update();

        boundary.load(model);
        sesph.load(model);
        advect.load(model);
      }

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
