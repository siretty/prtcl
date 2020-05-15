#pragma once

#include <prtcl/rt/basic_bcc_lattice_source.hpp>
#include <prtcl/rt/basic_fcc_lattice_source.hpp>
#include <prtcl/rt/basic_hcp_lattice_source.hpp>
#include <prtcl/rt/basic_model_policy.hpp>
#include <prtcl/rt/basic_scg_lattice_source.hpp>
#include <prtcl/rt/basic_source.hpp>
#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/cli/load_groups.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>
#include <prtcl/rt/filesystem/getcwd.hpp>
#include <prtcl/rt/geometry/axis_aligned_box.hpp>
#include <prtcl/rt/geometry/triangle_mesh.hpp>
#include <prtcl/rt/grouped_uniform_grid.hpp>
#include <prtcl/rt/integral_grid.hpp>
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

#include <prtcl/core/log/logger.hpp>

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
  using prtcl::core::dtype;

  static constexpr size_t N = ModelPolicy_::dimensionality;

  using type_policy = typename ModelPolicy_::type_policy;
  using math_policy = typename ModelPolicy_::math_policy;
  using o = typename math_policy::operations;

  using rvec = typename math_policy::template ndtype_t<dtype::real, N>;

  auto const h = model.template get_global<dtype::real>("smoothing_scale")[0];

  std::mt19937 gen;
  std::uniform_real_distribution<typename type_policy::real> dis{-0.01f, 0.01f};

  auto random_rvec = [&gen, &dis]() {
    rvec result;
    for (int d = 0; d < result.size(); ++d)
      result[d] = dis(gen);
    return result;
  };

  for (auto &group : model.groups()) {
    if (group.get_type() != "fluid")
      continue;

    auto const rho0 =
        group.template get_uniform<dtype::real>("rest_density")[0];

    auto const base_m = prtcl::core::constpow(h, N) * rho0;

    auto x = group.template get_varying<dtype::real, N>("position");
    auto v = group.template get_varying<dtype::real, N>("velocity");
    auto m = group.template get_varying<dtype::real>("mass");

    for (size_t i = 0; i < group.size(); ++i) {
      x[i] += h * random_rvec();
      v[i] = o::template zeros<dtype::real, N>();
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
  prtcl::rt::save_vtk(file, group);
  core::log::debug("app", "save_vtk", "saved ", file_path);
}

template <typename ModelPolicy_> class basic_application {
public:
  using base_type = basic_application;

  using model_policy = ModelPolicy_;
  using model_type = basic_model<model_policy>;
  using group_type = basic_group<model_policy>;
  using source_type = basic_hcp_lattice_source<model_policy>;

  // using neighborhood_type =
  //    prtcl::rt::neighbourhood<prtcl::rt::compact_n_search_grid<model_policy>>;
  using neighborhood_type =
      prtcl::rt::neighbourhood<prtcl::rt::grouped_uniform_grid<model_policy>>;

public:
  virtual ~basic_application() {}

protected:
  virtual void on_require_schemes(model_type &) {}
  virtual void on_load_schemes(model_type &) {}

  virtual void on_prepare_simulation(model_type &, neighborhood_type &) {
    core::log::get_logger().change(core::log::log_level::debug, false);
  }

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

  using o = typename math_policy::operations;

  using triangle_mesh_type = prtcl::rt::triangle_mesh<model_policy>;

  using dtype = prtcl::core::dtype;

  using real = typename type_policy::real;
  using rvec = typename math_policy::template ndtype_t<dtype::real, N>;

  using scheduler_type = virtual_scheduler<real>;
  using duration = typename scheduler_type::duration;

private:
  template <typename RemoveIdxs_, typename PositionInDomain_>
  void do_frame(
      size_t frame, model_type &model, neighborhood_type &nhood,
      scheduler_type &scheduler, long double seconds_per_frame,
      RemoveIdxs_ &remove_idxs, PositionInDomain_ &position_in_domain) {
    namespace log = prtcl::core::log;

    log::info_raii("app", "frame", '#', frame);

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

        auto x = group.template get_varying<dtype::real, N>("position");

#pragma omp parallel for
        for (size_t i = 0; i < x.size(); ++i) {
          if (not position_in_domain(x[i])) {
#pragma omp critical
            remove_idxs.push_back(i);
          }
        }

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

    auto h = model.template get_global<dtype::real>("smoothing_scale")[0];

    auto &clock = scheduler.clock();
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

      model.template get_global<dtype::real>("maximum_speed")[0] = 0;

      // run all computational steps
      this->do_prepare_step(model, nhood);
      this->do_step(model, nhood);
      this->do_step_done(model, nhood);

      auto dt = static_cast<long double>(
          model.template get_global<dtype::real>("time_step")[0]);

      // advance the simulation clock and run schedule
      clock.advance(static_cast<real>(dt));
      scheduler.tick();

      adjust_time_step(model);

      // set the current time
      model.template get_global<dtype::real>("current_time")[0] =
          clock.now().time_since_epoch().count();
    }

    log::debug(
        "app", "main", "current max_speed = ",
        model.template get_global<dtype::real>("maximum_speed")[0]);
    log::debug(
        "app", "main", "current time_step = ",
        model.template get_global<dtype::real>("time_step")[0]);

    PRTCL_RT_LOG_TRACE_CFRAME_MARK();
  }

  void adjust_time_step(model_type &model) {
    namespace log = prtcl::core::log;

    // fetch the maximum time step
    auto const max_time_step =
        model.template get_global<dtype::real>("maximum_time_step")[0];
    // fetch the minimum time step
    auto const min_time_step =
        model.template get_global<dtype::real>("minimum_time_step")[0];
    // fetch the smoothing scale of the simulation
    auto const h = model.template get_global<dtype::real>("smoothing_scale")[0];
    // fetch the maximum speed of any fluid particle
    auto const max_speed =
        model.template get_global<dtype::real>("maximum_speed")[0];
    // fetch the maximum allowed cfl number used to compute the time step
    auto const max_cfl =
        model.template get_global<dtype::real>("maximum_cfl")[0];

    // modify the timestep
    auto next_time_step = max_time_step;
    if (max_speed > 0)
      next_time_step = std::max(
          std::min(max_cfl * h / max_speed, max_time_step), min_time_step);
    model.template get_global<dtype::real>("time_step")[0] = next_time_step;

    if (next_time_step < max_time_step)
      log::debug(
          "app", "main", "REDUCED TIME STEP ",
          model.template get_global<dtype::real>("time_step")[0]);
  }

public:
  int main(int argc_, char **argv_) {
    namespace log = core::log;

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

    rvec b_lo = o::template most_positive<dtype::real, N>(),
         b_hi = o::template most_negative<dtype::real, N>();

    { // compute scene aabb
      // {{{
      for (auto &group : model.groups()) {
        if (not group.template has_varying<dtype::real, N>("position"))
          continue;

        auto x = group.template get_varying<dtype::real, N>("position");
        for (size_t i = 0; i < group.size(); ++i) {
          for (int n = 0; n < static_cast<int>(N); ++n) {
            b_lo[n] = std::min(b_lo[n], x[i][n]);
            b_hi[n] = std::max(b_hi[n], x[i][n]);
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
        model.template get_global<dtype::real>("smoothing_scale")[0]));

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

    auto const max_seconds = [&model] {
      if (model.template has_global<dtype::real>("maximum_simulation_seconds"))
        return model.template get_global<dtype::real>(
            "maximum_simulation_seconds")[0];
      else
        return real{1};
    }();

    size_t const fps = 30;
    size_t const max_frame = static_cast<size_t>(std::round(max_seconds * fps));

    auto const seconds_per_frame = (1.0L / fps);

    // set the current time and the fade duration
    model.template add_global<dtype::real>("current_time")[0] =
        clock.now().time_since_epoch().count();
    model.template add_global<dtype::real>("fade_duration")[0] =
        static_cast<real>(2 * seconds_per_frame);

    for (size_t frame = 0; frame <= max_frame; ++frame) {
      for (auto &group : model.groups()) {
        if ("fluid" != group.get_type())
          continue;

        save_group(output_dir, group, frame);
      }

      if (frame < max_frame)
        this->do_frame(
            frame, model, nhood, scheduler, seconds_per_frame, remove_idxs,
            position_in_domain);
    }

    log::info("app", "loop", "maximum frame #", max_frame, " reached");

    return 0;
  }
};

} // namespace prtcl::rt
