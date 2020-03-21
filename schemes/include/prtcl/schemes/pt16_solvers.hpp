#pragma once

#include <prtcl/rt/common.hpp>

#include <prtcl/rt/basic_group.hpp>
#include <prtcl/rt/basic_model.hpp>

#include <prtcl/rt/log/trace.hpp>

#include <prtcl/core/log/logger.hpp>

#include <vector>

#include <omp.h>

#include <Eigen/Eigen>

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

namespace prtcl {
namespace schemes {

template <typename ModelPolicy_> class pt16_solvers {
public:
  using model_policy = ModelPolicy_;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;
  using data_policy = typename model_policy::data_policy;

  using dtype = prtcl::rt::dtype;

  template <dtype DType_>
  using dtype_t = typename type_policy::template dtype_t<DType_>;
  template <dtype DType_, size_t... Ns_>
  using ndtype_t = typename math_policy::template ndtype_t<DType_, Ns_...>;
  template <dtype DType_, size_t... Ns_>
  using ndtype_data_ref_t =
      typename data_policy::template ndtype_data_ref_t<DType_, Ns_...>;

  static constexpr size_t N = model_policy::dimensionality;

  using model_type = prtcl::rt::basic_model<model_policy>;
  using group_type = prtcl::rt::basic_group<model_policy>;

private:
  struct global_data {
    ndtype_data_ref_t<dtype::real> smoothing_scale;
    ndtype_data_ref_t<dtype::real> current_time;
    ndtype_data_ref_t<dtype::real> fade_duration;

    static void _require(model_type &m_) {
      m_.template add_global<dtype::real>("smoothing_scale");
      m_.template add_global<dtype::real>("current_time");
      m_.template add_global<dtype::real>("fade_duration");
    }

    void _load(model_type const &m_) {
      smoothing_scale = m_.template get_global<dtype::real>("smoothing_scale");
      current_time = m_.template get_global<dtype::real>("current_time");
      fade_duration = m_.template get_global<dtype::real>("fade_duration");
    }
  };

private:
  struct fluid_data {
    // particle count of the selected group
    size_t _count;
    // index of the selected group
    size_t _index;

    // uniform fields
    ndtype_data_ref_t<dtype::real> rest_density;
    ndtype_data_ref_t<dtype::real> pt16_vorticity_diffusion_maximum_error;
    ndtype_data_ref_t<dtype::integer>
        pt16_vorticity_diffusion_maximum_iterations;
    ndtype_data_ref_t<dtype::real> pt16_velocity_reconstruction_maximum_error;
    ndtype_data_ref_t<dtype::integer>
        pt16_velocity_reconstruction_maximum_iterations;

    // varying fields
    ndtype_data_ref_t<dtype::real> mass;
    ndtype_data_ref_t<dtype::real, N> vorticity;
    ndtype_data_ref_t<dtype::real> pt16_vorticity_diffusion_diagonal;
    ndtype_data_ref_t<dtype::real, N> pt16_vorticity_diffusion_rhs;
    ndtype_data_ref_t<dtype::real> density;
    ndtype_data_ref_t<dtype::real, N, N> target_velocity_gradient;
    ndtype_data_ref_t<dtype::real, N> velocity;
    ndtype_data_ref_t<dtype::real> pt16_velocity_reconstruction_diagonal;
    ndtype_data_ref_t<dtype::real, N> pt16_velocity_reconstruction_rhs;
    ndtype_data_ref_t<dtype::real, N> position;
    ndtype_data_ref_t<dtype::real> time_of_birth;

    static void _require(group_type &g_) {
      // uniform fields
      g_.template add_uniform<dtype::real>("rest_density");
      g_.template add_uniform<dtype::real>(
          "pt16_vorticity_diffusion_maximum_error");
      g_.template add_uniform<dtype::integer>(
          "pt16_vorticity_diffusion_maximum_iterations");
      g_.template add_uniform<dtype::real>(
          "pt16_velocity_reconstruction_maximum_error");
      g_.template add_uniform<dtype::integer>(
          "pt16_velocity_reconstruction_maximum_iterations");

      // varying fields
      g_.template add_varying<dtype::real>("mass");
      g_.template add_varying<dtype::real, N>("vorticity");
      g_.template add_varying<dtype::real>("pt16_vorticity_diffusion_diagonal");
      g_.template add_varying<dtype::real, N>("pt16_vorticity_diffusion_rhs");
      g_.template add_varying<dtype::real>("density");
      g_.template add_varying<dtype::real, N, N>("target_velocity_gradient");
      g_.template add_varying<dtype::real, N>("velocity");
      g_.template add_varying<dtype::real>(
          "pt16_velocity_reconstruction_diagonal");
      g_.template add_varying<dtype::real, N>(
          "pt16_velocity_reconstruction_rhs");
      g_.template add_varying<dtype::real, N>("position");
      g_.template add_varying<dtype::real>("time_of_birth");
    }

    void _load(group_type const &g_) {
      _count = g_.size();

      // uniform fields
      rest_density = g_.template get_uniform<dtype::real>("rest_density");
      pt16_vorticity_diffusion_maximum_error =
          g_.template get_uniform<dtype::real>(
              "pt16_vorticity_diffusion_maximum_error");
      pt16_vorticity_diffusion_maximum_iterations =
          g_.template get_uniform<dtype::integer>(
              "pt16_vorticity_diffusion_maximum_iterations");
      pt16_velocity_reconstruction_maximum_error =
          g_.template get_uniform<dtype::real>(
              "pt16_velocity_reconstruction_maximum_error");
      pt16_velocity_reconstruction_maximum_iterations =
          g_.template get_uniform<dtype::integer>(
              "pt16_velocity_reconstruction_maximum_iterations");

      // varying fields
      mass = g_.template get_varying<dtype::real>("mass");
      vorticity = g_.template get_varying<dtype::real, N>("vorticity");
      pt16_vorticity_diffusion_diagonal = g_.template get_varying<dtype::real>(
          "pt16_vorticity_diffusion_diagonal");
      pt16_vorticity_diffusion_rhs = g_.template get_varying<dtype::real, N>(
          "pt16_vorticity_diffusion_rhs");
      density = g_.template get_varying<dtype::real>("density");
      target_velocity_gradient = g_.template get_varying<dtype::real, N, N>(
          "target_velocity_gradient");
      velocity = g_.template get_varying<dtype::real, N>("velocity");
      pt16_velocity_reconstruction_diagonal =
          g_.template get_varying<dtype::real>(
              "pt16_velocity_reconstruction_diagonal");
      pt16_velocity_reconstruction_rhs =
          g_.template get_varying<dtype::real, N>(
              "pt16_velocity_reconstruction_rhs");
      position = g_.template get_varying<dtype::real, N>("position");
      time_of_birth = g_.template get_varying<dtype::real>("time_of_birth");
    }
  };

public:
  static void require(model_type &m_) {
    global_data::_require(m_);

    for (auto &group : m_.groups()) {
      if ((group.get_type() == "fluid") and (true)) {
        fluid_data::_require(group);
      }
    }
  }

public:
  void load(model_type &m_) {
    _group_count = m_.groups().size();

    _data.global._load(m_);

    _data.by_group_type.fluid.clear();

    auto groups = m_.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      auto &group =
          groups[static_cast<typename decltype(groups)::difference_type>(i)];

      if ((group.get_type() == "fluid") and (true)) {
        auto &data = _data.by_group_type.fluid.emplace_back();
        data._load(group);
        data._index = i;
      }
    }
  }

private:
  struct {
    global_data global;
    struct {
      std::vector<fluid_data> fluid;
    } by_group_type;
  } _data;

  struct per_thread_type {
    std::vector<std::vector<size_t>> neighbors;

    // reductions
  };

  std::vector<per_thread_type> _per_thread;
  size_t _group_count;

  using real = dtype_t<dtype::real>;

public:
  template <typename NHood_> size_t vorticity_diffusion(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member types
    using o = typename math_policy::operations;

    size_t d = 0;

    auto diagonal = [](auto &p, size_t f, auto &) {
      return p.pt16_vorticity_diffusion_diagonal[f];
    };

    auto product = [&g, &diagonal](
                       auto &p, size_t f, auto &neighbors, auto const &x) {
      auto const h = g.smoothing_scale[0];

      real result = diagonal(p, f, neighbors) * x(f);

      for (auto f_f : neighbors[p._index]) {
        if (f != f_f)
          result -= p.mass[f_f] *
                    o::kernel_h(p.position[f] - p.position[f_f], h) * x(f_f);
      }

      return result;
    };

    auto rhs = [&d](auto &p, size_t f, auto &) {
      return p.pt16_vorticity_diffusion_rhs[f][d];
    };

    auto iterate = [&d](auto &p, size_t f) -> auto & {
      return p.vorticity[f][d];
    };

    size_t iterations = 0;

    for (auto &p : _data.by_group_type.fluid) {
      auto max_error = p.pt16_vorticity_diffusion_maximum_error[0];
      auto max_iters = p.pt16_vorticity_diffusion_maximum_iterations[0];
      for (d = 0; d < N; ++d) {
        iterations += math_policy::solve_cg_dp(
            nhood_, _group_count, p, iterate, product, rhs, diagonal,
            max_error * 1e-5 * p.rest_density[0], max_iters);
      }
    }

    return iterations;
  }

public:
  template <typename NHood_>
  size_t velocity_reconstruction(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member types
    using o = typename math_policy::operations;

    size_t d = 0;

    auto diagonal = [&g](auto &p, size_t f, auto &) {
      return p.pt16_velocity_reconstruction_diagonal[f];
    };

    auto product = [&g, &diagonal](
                       auto &p, size_t f, auto &neighbors, auto const &x) {
      auto const h = g.smoothing_scale[0];

      real result = diagonal(p, f, neighbors) * x(f);

      for (auto f_f : neighbors[p._index]) {
        if (f != f_f)
          result -= p.mass[f_f] *
                    o::kernel_h(p.position[f] - p.position[f_f], h) * x(f_f);
      }

      return result;
    };

    auto rhs = [&d](auto &p, size_t f, auto &) {
      return p.pt16_velocity_reconstruction_rhs[f][d];
    };

    auto iterate = [&d](auto &p, size_t f) -> auto & {
      return p.velocity[f][d];
    };

    auto apply = [&g](auto &p, size_t f, real old, real new_) -> real {
      if (g.current_time[0] - p.time_of_birth[f] > g.fade_duration[0])
        return new_;
      else
        return old;
    };

    size_t iterations = 0;

    for (auto &p : _data.by_group_type.fluid) {
      auto max_error = p.pt16_velocity_reconstruction_maximum_error[0];
      auto max_iters = p.pt16_velocity_reconstruction_maximum_iterations[0];
      for (d = 0; d < N; ++d) {
        size_t d_iterations = math_policy::solve_cg_dp(
            nhood_, _group_count, p, iterate, product, rhs, diagonal,
            max_error * 1e-5 * p.rest_density[0], max_iters, apply);
        prtcl::core::log::debug(
            "app", "pt16", "dim=", d, " iterations ", d_iterations);
        iterations += d_iterations;
      }
    }

    return iterations;
  }
};

} // namespace schemes
} // namespace prtcl

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
