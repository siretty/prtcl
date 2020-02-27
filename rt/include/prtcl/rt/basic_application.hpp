#pragma once

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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <optional>
#include <random>
#include <tuple>

#include <cstdlib>

#include <unistd.h>

namespace prtcl::rt {

template <typename Model_, typename... Schemes_>
void require_all(Model_ &model, Schemes_ &... schemes_) {
  // call require for all schemes
  (schemes_.require(model), ...);
}

template <typename Model_, typename... Schemes_>
void load_all(Model_ &model, Schemes_ &... schemes_) {
  // call require for all schemes
  (schemes_.load(model), ...);
}

template <typename ModelPolicy_>
void initialize_fluid(prtcl::rt::basic_model<ModelPolicy_> &model) {
  // {{{
  using prtcl::core::nd_dtype;

  static constexpr size_t N = ModelPolicy_::dimensionality;

  using type_policy = typename ModelPolicy_::type_policy;
  using math_policy = typename ModelPolicy_::math_policy;
  using c = typename math_policy::constants;

  using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, N>;

  auto const h =
      model.template get_global<nd_dtype::real>("smoothing_scale")[0];

  std::mt19937 gen;
  std::uniform_real_distribution<typename type_policy::real> dis{-0.01f, 0.01f};

  auto random_rvec = [&gen, &dis]() {
    rvec result;
    for (size_t i = 0; i < result.size(); ++i)
      result[i] = dis(gen);
    return result;
  };

  for (auto &group : model.groups()) {
    if (group.get_type() != "fluid")
      continue;

    auto const rho0 =
        group.template get_uniform<nd_dtype::real>("rest_density")[0];

    auto const base_m = prtcl::core::constpow(h, N) * rho0;

    auto x = group.template get_varying<nd_dtype::real, N>("position");
    auto v = group.template get_varying<nd_dtype::real, N>("velocity");
    auto m = group.template get_varying<nd_dtype::real>("mass");

    for (size_t i = 0; i < group.size(); ++i) {
      x[i] += h * random_rvec();
      v[i] = c::template zeros<nd_dtype::real, N>();
      m[i] = base_m + dis(gen) * base_m / 10;
    }
  }
  // }}}
}

template <typename ModelPolicy_>
void save_group(
    std::string output_dir, prtcl::rt::basic_group<ModelPolicy_> const &group,
    std::optional<size_t> frame = std::nullopt) {
  auto file_name =
      std::string{group.get_name()} +
      (frame ? "." + std::to_string(frame.value()) : std::string{}) + ".vtk";
  auto file_path = output_dir + "/" + file_name;
  std::ofstream file{file_path, file.trunc | file.out};
  std::cout << "START SAVE_VTK " << output_dir << '\n';
  prtcl::rt::save_vtk(file, group);
  std::cout << "CLOSE SAVE_VTK " << file_path << '\n';
}

template <typename ModelPolicy_> class basic_application {
public:
  using base_type = basic_application;

  using model_policy = ModelPolicy_;
  using model_type = prtcl::rt::basic_model<model_policy>;
  using neighborhood_type =
      prtcl::rt::neighbourhood<prtcl::rt::grouped_uniform_grid<model_policy>>;

public:
  virtual ~basic_application() {}

protected:
  virtual void on_require_schemes(model_type &) {}
  virtual void on_load_schemes(model_type &) {}

  virtual void on_prepare_simulation(model_type &, neighborhood_type &) {}

  virtual void on_prepare_frame(model_type &, neighborhood_type &) {}
  virtual void on_prepare_step(model_type &, neighborhood_type &) {}
  virtual void on_step(model_type &, neighborhood_type &) {}
  virtual void on_step_done(model_type &, neighborhood_type &) {}
  virtual void on_frame_done(model_type &, neighborhood_type &) {}

private:
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;

  static constexpr size_t N = model_policy::dimensionality;

  using c = typename math_policy::constants;
  using o = typename math_policy::operations;

  using triangle_mesh_type = prtcl::rt::triangle_mesh<model_policy>;

  using nd_dtype = prtcl::core::nd_dtype;

  using real = typename type_policy::real;
  using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, N>;

public:
  int main(int argc_, char **argv_) {
    prtcl::rt::command_line_interface cli{argc_, argv_};

    // dump parameters and scene description
    boost::property_tree::write_json(std::cout, cli.name_value());

    model_type model;

    prtcl::rt::load_model_groups_from_cli(cli, model);

    this->on_require_schemes(model);

    // -----

    initialize_fluid(model);

    rvec b_lo = c::template most_positive<nd_dtype::real, N>(),
         b_hi = c::template most_negative<nd_dtype::real, N>();

    { // compute boundary aabb
      // {{{
      for (auto &group : model.groups()) {
        if (group.get_type() != "boundary")
          continue;

        auto x_b = group.template get_varying<nd_dtype::real, N>("position");
        for (size_t i = 0; i < group.size(); ++i) {
          for (int n = 0; n < N; ++n) {
            b_lo[n] = std::min(b_lo[n], x_b[i][n]);
            b_hi[n] = std::max(b_hi[n], x_b[i][n]);
          }
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

    neighborhood_type nhood;
    nhood.set_radius(math_policy::operations::kernel_support_radius(
        model.template get_global<nd_dtype::real>("smoothing_scale")[0]));

    nhood.load(model);
    nhood.update();

    nhood.permute(model);

    this->on_load_schemes(model);

    this->on_prepare_simulation(model, nhood);

    auto cwd = ::prtcl::rt::filesystem::getcwd();
    auto output_dir = cwd + "/" + "output";

    for (auto &group : model.groups()) {
      if ("boundary" != group.get_type())
        continue;

      save_group(output_dir, group);
    }

    std::vector<size_t> remove_idxs, remove_perm;
    std::vector<rvec> spawned_positions, spawned_velocities;

    size_t const fps = 30;
    size_t const max_frame = 60 * fps;

    auto clock = std::make_shared<prtcl::rt::virtual_clock<long double>>();
    using duration = typename decltype(clock)::element_type::duration;

    auto const seconds_per_frame = (1.0L / fps);

    auto const max_cfl =
        model.template get_global<nd_dtype::real>("maximum_cfl")[0];
    auto const max_time_step =
        model.template get_global<nd_dtype::real>("maximum_time_step")[0];

    for (size_t frame = 0; frame <= max_frame; ++frame) {
      for (auto &group : model.groups()) {
        if ("fluid" != group.get_type())
          continue;

        save_group(output_dir, group, frame);
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

          auto x = group.template get_varying<nd_dtype::real, N>("position");
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
          model.template get_global<nd_dtype::real>("smoothing_scale")[0]);

      auto frame_done = clock->now() + duration{seconds_per_frame};

      std::cout << "START OF FRAME #" << frame + 1 << '\n';
      while (clock->now() < frame_done) {
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
              real min_distance =
                  c::template positive_infinity<nd_dtype::real>();
              nhood.neighbors(
                  x[i], [&model, &min_distance, &x, i](size_t g, size_t j) {
                    auto &g_n = model.get_group(g);
                    auto x_n =
                        g_n.template get_varying<nd_dtype::real, N>("position");
                    min_distance =
                        std::min(min_distance, o::norm(x[i] - x_n[j]));
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

          auto rho0 =
              group.template get_uniform<nd_dtype::real>("rest_density")[0];
          auto x = group.template get_varying<nd_dtype::real, N>("position");
          auto v = group.template get_varying<nd_dtype::real, N>("velocity");
          auto m = group.template get_varying<nd_dtype::real>("mass");

          for (size_t i = 0; i < spawned_positions.size(); ++i) {
            x[indices[i]] = spawned_positions[i];
            v[indices[i]] = spawned_velocities[i];
            m[indices[i]] =
                static_cast<real>(prtcl::core::constpow(h, N)) * rho0;
          }

          // std::cerr << "spawned " << spawned_positions.size()
          //          << " particles in group " << group.get_name() <<
          //          std::endl;

          // }}}
        }

        // if new particles were spawned, reload the neighborhood and the
        // schemes
        if (spawned) {
          // std::cerr << "reloading neighborhood and schemes" << std::endl;

          nhood.load(model);
          nhood.update();

          this->on_load_schemes(model);
        }

        model.template get_global<nd_dtype::real>("maximum_speed")[0] = 0;

        // run all computational steps
        this->on_step(model, nhood);

        // fetch the maximum speed of any fluid particle
        auto max_speed = static_cast<long double>(
            model.template get_global<nd_dtype::real>("maximum_speed")[0]);

        auto dt = static_cast<long double>(
            model.template get_global<nd_dtype::real>("time_step")[0]);

        // advance the simulation clock
        clock->advance(dt);

        // modify the timestep
        model.template get_global<nd_dtype::real>("time_step")[0] =
            std::min<real>(max_cfl * h / max_speed, max_time_step);
      }

      std::cerr << "current max_speed = "
                << model.template get_global<nd_dtype::real>("maximum_speed")[0]
                << std::endl;
      std::cerr << "current time_step = "
                << model.template get_global<nd_dtype::real>("time_step")[0]
                << std::endl;

      std::cout << "CLOSE OF FRAME #" << frame + 1 << '\n' << '\n';
    }
  }
};

} // namespace prtcl::rt
