
#include "prtcl/rt/geometry/axis_aligned_box.hpp"
#include <iterator>
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

#include <prtcl/rt/cli/command_line_interface.hpp>

#include <prtcl/rt/sample_surface.hpp>
#include <prtcl/rt/sample_volume.hpp>
#include <prtcl/rt/triangle_mesh.hpp>

#include <prtcl/schemes/boundary.hpp>
#include <prtcl/schemes/sesph_v2.hpp>
#include <prtcl/schemes/symplectic_euler.hpp>

#include <fstream>
#include <iostream>
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

using triangle_mesh_type = prtcl::rt::triangle_mesh<model_policy>;

using real = typename type_policy::real;
using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, N>;

int main(int argc_, char **argv_) {
  prtcl::rt::command_line_interface cli{argc_, argv_};

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

  auto &f = model.add_group("f", "fluid");
  auto &b = model.add_group("b", "boundary");
  auto &c = model.add_group("c", "boundary");

  prtcl::schemes::boundary<model_policy> boundary;
  boundary.require(model);

  prtcl::schemes::sesph<model_policy> sesph;
  sesph.require(model);

  prtcl::schemes::symplectic_euler<model_policy> advect;
  advect.require(model);

  { // initialize global and uniform fields
    auto const h = cli.value_with_name_or<real>(
        "parameter.smoothing_scale", static_cast<real>(0.025));
    auto const rho0 = cli.value_with_name_or<real>(
        "parameter.rest_density", static_cast<real>(1000));

    model.get_global<nd_dtype::real>("smoothing_scale")[0] = h;
    model.get_global<nd_dtype::real>("time_step")[0] = 0.00001f;

    model.get_global<nd_dtype::real, N>("gravity")[0] =
        math_policy::constants::zeros<nd_dtype::real, N>();
    model.get_global<nd_dtype::real, N>("gravity")[0][1] = -9.81f;

    f.get_uniform<nd_dtype::real>("rest_density")[0] = rho0;
    f.get_uniform<nd_dtype::real>("compressibility")[0] = 10'000'000;
    f.get_uniform<nd_dtype::real>("viscosity")[0] = 0.01f;
  }

  // -----

  size_t grid_size = 16;

  // {{{
  { // initialize fluid position and velocity
    using c = typename math_policy::constants;

    auto const h = model.get_global<nd_dtype::real>("smoothing_scale")[0];
    auto const rho0 = f.get_uniform<nd_dtype::real>("rest_density")[0];

    auto const base_m = prtcl::core::constpow(h, N) * rho0;

    std::mt19937 gen;
    std::uniform_real_distribution<typename type_policy::real> dis{-0.01f,
                                                                   0.01f};

    auto const lo_x = cli.value_with_name_or("fluid.aab.lo.x", 1);
    auto const lo_y = cli.value_with_name_or("fluid.aab.lo.y", 1);
    auto const lo_z = cli.value_with_name_or("fluid.aab.lo.z", 1);

    auto const hi_x = cli.value_with_name_or("fluid.aab.hi.x", grid_size);
    auto const hi_y = cli.value_with_name_or("fluid.aab.hi.y", 2 * grid_size);
    auto const hi_z = cli.value_with_name_or("fluid.aab.hi.z", grid_size);

    prtcl::rt::axis_aligned_box<model_policy, N> aab{
        h * rvec{lo_x, lo_y, lo_z}, h * rvec{hi_x, hi_y, hi_z}};

    std::vector<rvec> samples;
    prtcl::rt::sample_volume(aab, std::back_inserter(samples), {h});

    f.resize(samples.size());

    auto x = f.get_varying<nd_dtype::real, N>("position");
    auto v = f.get_varying<nd_dtype::real, N>("velocity");
    auto m = f.get_varying<nd_dtype::real>("mass");

    for (size_t i = 0; i < f.size(); ++i) {
      x[i] = samples[i] + h * rvec{dis(gen), dis(gen), dis(gen)};
      v[i] = c::zeros<nd_dtype::real, N>();
      m[i] = base_m + dis(gen) * base_m / 10;
    }
  }
  // }}}

  // {{{
  rvec b_lo = c::most_positive<nd_dtype::real, N>(),
       b_hi = c::most_negative<nd_dtype::real, N>();

  { // initialize boundary position
    auto const h = model.get_global<nd_dtype::real>("smoothing_scale")[0];

    auto const t_f = real{-2};
    auto const t_x = cli.value_with_name_or("boundary.translate.x", t_f);
    auto const t_y = cli.value_with_name_or("boundary.translate.y", t_f);
    auto const t_z = cli.value_with_name_or("boundary.translate.z", t_f);
    rvec const t{t_x, t_y, t_z};

    auto const s_f = real{2} * grid_size;
    auto const s_x = cli.value_with_name_or("boundary.scale.x", s_f);
    auto const s_y = cli.value_with_name_or("boundary.scale.y", 2 * s_f);
    auto const s_z = cli.value_with_name_or("boundary.scale.z", s_f);
    rvec const s{s_x, s_y, s_z};

    boundary_mesh.scale(h * s);
    boundary_mesh.translate(h * t);

    std::vector<rvec> samples;
    prtcl::rt::sample_surface(boundary_mesh, std::back_inserter(samples), {h});

    std::cerr << "#samples = " << samples.size() << '\n';

    b.resize(samples.size());
    auto x_b = b.get_varying<nd_dtype::real, N>("position");

    for (size_t ix = 0; ix < samples.size(); ++ix) {
      x_b[ix] = samples[ix];

      for (int n = 0; n < N; ++n) {
        b_lo[n] = std::min(b_lo[n], x_b[ix][n]);
        b_hi[n] = std::max(b_hi[n], x_b[ix][n]);
      }
    }
  }

  { // setup the catch-box
    triangle_mesh_type c_mesh;
    { // open the geometry file for reading
      std::ifstream file{"share/models/unitcube.obj", std::ios::in};
      // read the mesh from the file
      c_mesh = triangle_mesh_type::load_from_obj(file);
      // close the file
      file.close();
    }

    auto const h = model.get_global<nd_dtype::real>("smoothing_scale")[0];

    c_mesh.scale(4 * (b_hi - b_lo));
    c_mesh.translate(b_lo - 1.5 * (b_hi - b_lo));

    std::vector<rvec> samples;
    prtcl::rt::sample_surface(c_mesh, std::back_inserter(samples), {h});

    c.resize(samples.size());
    auto x_c = c.get_varying<nd_dtype::real, N>("position");

    for (size_t ix = 0; ix < c.size(); ++ix)
      x_c[ix] = samples[ix];
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
