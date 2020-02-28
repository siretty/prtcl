#pragma once

#include <prtcl/rt/basic_model_policy.hpp>
#include <prtcl/rt/basic_source.hpp>
#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/cli/load_groups.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>
#include <prtcl/rt/external/compact_n_search_grid.hpp>
#include <prtcl/rt/filesystem/getcwd.hpp>
#include <prtcl/rt/geometry/axis_aligned_box.hpp>
#include <prtcl/rt/geometry/triangle_mesh.hpp>
#include <prtcl/rt/grouped_uniform_grid.hpp>
#include <prtcl/rt/integral_grid.hpp>
#include <prtcl/rt/log/logger.hpp>
#include <prtcl/rt/log/trace.hpp>
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

#include <boost/algorithm/cxx11/any_of.hpp>

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
  PRTCL_RT_LOG_TRACE_SCOPED(
      "save_group", "group=", group.get_name(),
      " frame=", (frame ? std::to_string(frame.value()) : std::string{"-"}));

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
  using model_type = basic_model<model_policy>;
  using group_type = basic_group<model_policy>;
  using source_type = basic_source<model_policy>;

  // using neighborhood_type =
  //    prtcl::rt::neighbourhood<prtcl::rt::compact_n_search_grid<model_policy>>;
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
  void do_require_schemes(model_type &model) {
    PRTCL_RT_LOG_TRACE_SCOPED("do_require_schemes");
    this->on_require_schemes(model);
  }

  void do_load_schemes(model_type &model) {
    PRTCL_RT_LOG_TRACE_SCOPED("do_load_schemes");
    this->on_load_schemes(model);
  }

  void do_prepare_simulation(model_type &model, neighborhood_type &nhood) {
    PRTCL_RT_LOG_TRACE_SCOPED("do_prepare_simulation");
    this->on_prepare_simulation(model, nhood);
  }

  void do_prepare_frame(model_type &model, neighborhood_type &nhood) {
    PRTCL_RT_LOG_TRACE_SCOPED("do_prepare_frame");
    this->on_prepare_frame(model, nhood);
  }

  void do_prepare_step(model_type &model, neighborhood_type &nhood) {
    PRTCL_RT_LOG_TRACE_SCOPED("do_prepare_step");
    this->on_prepare_step(model, nhood);
  }

  void do_step(model_type &model, neighborhood_type &nhood) {
    PRTCL_RT_LOG_TRACE_SCOPED("do_step");
    this->on_step(model, nhood);
  }

  void do_step_done(model_type &model, neighborhood_type &nhood) {
    PRTCL_RT_LOG_TRACE_SCOPED("do_step_done");
    this->on_step_done(model, nhood);
  }

  void do_frame_done(model_type &model, neighborhood_type &nhood) {
    PRTCL_RT_LOG_TRACE_SCOPED("do_frame_done");
    this->on_frame_done(model, nhood);
  }

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

  using scheduler_type = virtual_scheduler<real>;
  using duration = typename scheduler_type::duration;

public:
  int main(int argc_, char **argv_) {
    scheduler_type scheduler;
    auto &clock = scheduler.clock();

    // collect command line arguments
    prtcl::rt::command_line_interface cli{argc_, argv_};

    // dump parameters and scene description
    boost::property_tree::write_json(std::cout, cli.name_value());

    model_type model;
    std::vector<source_type> sources;

    prtcl::rt::load_model_groups_from_cli(
        cli, model, std::back_inserter(sources));

    log::info(
        "app", "basic_application", "registered ", sources.size(),
        " particle source(s)");

    // hand over all sources to the scheduler
    for (auto &source : sources)
      scheduler.schedule(duration{.1}, source);
    sources.clear();

    this->do_require_schemes(model);

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

    auto position_in_domain = [lo = rvec{b_lo - 5 * (b_hi - b_lo)},
                               hi = rvec{b_hi + 5 * (b_hi - b_lo)}] //
        (auto const &x) {
          return (lo.array() <= x.array()).all() and
                 (x.array() <= hi.array()).all();
        };

    // -----

    neighborhood_type nhood;
    nhood.set_radius(math_policy::operations::kernel_support_radius(
        model.template get_global<nd_dtype::real>("smoothing_scale")[0]));

    nhood.rebuild(model);

    this->do_load_schemes(model);

    this->do_prepare_simulation(model, nhood);

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

      // remove particles by permuting them to the end and resizing the storage
      if (0 == frame % 4) {
        bool particles_were_erased = false;

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

          if (remove_idxs.size() > 0) {
            group.erase(remove_idxs);
            particles_were_erased = true;
          }
        }
        // }}}

        // in case particles were erased ...
        if (particles_were_erased) {
          // ... and reload the schemes
          this->do_load_schemes(model);
          // ... rebuild the neighborhood
          nhood.rebuild(model);

          for (auto &group : model.groups())
            group.dirty(false);
        }
      }

      auto h = static_cast<long double>(
          model.template get_global<nd_dtype::real>("smoothing_scale")[0]);

      auto frame_done = clock.now() + duration{seconds_per_frame};

      while (clock.now() < frame_done) {
        PRTCL_RT_LOG_TRACE_SCOPED(
            "step", "t=", clock.now().time_since_epoch().count());

        if (boost::algorithm::any_of(model.groups(), [](auto const &group) {
              return group.dirty();
            })) {
          this->do_load_schemes(model);
          nhood.rebuild(model);

          for (auto &group : model.groups())
            group.dirty(false);
        } else {
          // update the neighborhood
          nhood.update();

          // permute the particles according to the neighborhood
          if (0 == frame % 8) {
            nhood.permute(model);
            nhood.update();
          }
        }

        model.template get_global<nd_dtype::real>("maximum_speed")[0] = 0;

        // run all computational steps
        this->do_step(model, nhood);

        // fetch the maximum speed of any fluid particle
        auto max_speed = static_cast<long double>(
            model.template get_global<nd_dtype::real>("maximum_speed")[0]);

        auto dt = static_cast<long double>(
            model.template get_global<nd_dtype::real>("time_step")[0]);

        // advance the simulation clock and run schedule
        clock.advance(dt);
        scheduler.tick();

        // modify the timestep
        auto next_time_step = max_time_step;
        if (max_speed > 0)
          next_time_step =
              std::min<real>(max_cfl * h / max_speed, max_time_step);
        model.template get_global<nd_dtype::real>("time_step")[0] =
            next_time_step;
      }

      log::debug(
          "app", "main", "current max_speed = ",
          model.template get_global<nd_dtype::real>("maximum_speed")[0]);
      log::debug(
          "app", "main", "current time_step = ",
          model.template get_global<nd_dtype::real>("time_step")[0]);

      PRTCL_RT_LOG_TRACE_CFRAME_MARK();
    }
  }
};

} // namespace prtcl::rt
