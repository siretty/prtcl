#pragma once

#include <prtcl/rt/common.hpp>

#include <prtcl/rt/basic_group.hpp>
#include <prtcl/rt/basic_model.hpp>

#include <prtcl/rt/log/logger.hpp>
#include <prtcl/rt/log/trace.hpp>

#include <vector>

#include <omp.h>

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

  using nd_dtype = prtcl::rt::nd_dtype;

  template <nd_dtype DType_>
  using dtype_t = typename type_policy::template dtype_t<DType_>;
  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_t = typename math_policy::template nd_dtype_t<DType_, Ns_...>;
  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_ref_t =
      typename data_policy::template nd_dtype_data_ref_t<DType_, Ns_...>;

  static constexpr size_t N = model_policy::dimensionality;

  using model_type = prtcl::rt::basic_model<model_policy>;
  using group_type = prtcl::rt::basic_group<model_policy>;

private:
  struct global_data {
    nd_dtype_data_ref_t<nd_dtype::real> smoothing_scale;
    nd_dtype_data_ref_t<nd_dtype::real> viscosity;

    static void _require(model_type &m_) {
      m_.template add_global<nd_dtype::real>("smoothing_scale");
      m_.template add_global<nd_dtype::real>("viscosity");
    }

    void _load(model_type const &m_) {
      smoothing_scale =
          m_.template get_global<nd_dtype::real>("smoothing_scale");
      viscosity = m_.template get_global<nd_dtype::real>("viscosity");
    }
  };

private:
  struct fluid_data {
    // particle count of the selected group
    size_t _count;
    // index of the selected group
    size_t _index;

    // uniform fields
    nd_dtype_data_ref_t<nd_dtype::real> rest_density;
    nd_dtype_data_ref_t<nd_dtype::real> viscosity;

    // varying fields
    nd_dtype_data_ref_t<nd_dtype::real> mass;
    nd_dtype_data_ref_t<nd_dtype::real, N> vorticity;
    nd_dtype_data_ref_t<nd_dtype::real> pt16_vorticity_diffusion_diagonal;
    nd_dtype_data_ref_t<nd_dtype::real, N> pt16_vorticity_diffusion_rhs;
    nd_dtype_data_ref_t<nd_dtype::real> density;
    nd_dtype_data_ref_t<nd_dtype::real, N, N> target_velocity_gradient;
    nd_dtype_data_ref_t<nd_dtype::real, N> velocity;
    nd_dtype_data_ref_t<nd_dtype::real> pt16_velocity_reconstruction_diagonal;
    nd_dtype_data_ref_t<nd_dtype::real, N> pt16_velocity_reconstruction_rhs;
    nd_dtype_data_ref_t<nd_dtype::real, N> position;

    static void _require(group_type &g_) {
      // uniform fields
      g_.template add_uniform<nd_dtype::real>("rest_density");
      g_.template add_uniform<nd_dtype::real>("viscosity");

      // varying fields
      g_.template add_varying<nd_dtype::real>("mass");
      g_.template add_varying<nd_dtype::real, N>("vorticity");
      g_.template add_varying<nd_dtype::real>(
          "pt16_vorticity_diffusion_diagonal");
      g_.template add_varying<nd_dtype::real, N>(
          "pt16_vorticity_diffusion_rhs");
      g_.template add_varying<nd_dtype::real>("density");
      g_.template add_varying<nd_dtype::real, N, N>("target_velocity_gradient");
      g_.template add_varying<nd_dtype::real, N>("velocity");
      g_.template add_varying<nd_dtype::real>(
          "pt16_velocity_reconstruction_diagonal");
      g_.template add_varying<nd_dtype::real, N>(
          "pt16_velocity_reconstruction_rhs");
      g_.template add_varying<nd_dtype::real, N>("position");
    }

    void _load(group_type const &g_) {
      _count = g_.size();

      // uniform fields
      rest_density = g_.template get_uniform<nd_dtype::real>("rest_density");
      viscosity = g_.template get_uniform<nd_dtype::real>("viscosity");

      // varying fields
      mass = g_.template get_varying<nd_dtype::real>("mass");
      vorticity = g_.template get_varying<nd_dtype::real, N>("vorticity");
      pt16_vorticity_diffusion_diagonal =
          g_.template get_varying<nd_dtype::real>(
              "pt16_vorticity_diffusion_diagonal");
      pt16_vorticity_diffusion_rhs = g_.template get_varying<nd_dtype::real, N>(
          "pt16_vorticity_diffusion_rhs");
      density = g_.template get_varying<nd_dtype::real>("density");
      target_velocity_gradient = g_.template get_varying<nd_dtype::real, N, N>(
          "target_velocity_gradient");
      velocity = g_.template get_varying<nd_dtype::real, N>("velocity");
      pt16_velocity_reconstruction_diagonal =
          g_.template get_varying<nd_dtype::real>(
              "pt16_velocity_reconstruction_diagonal");
      pt16_velocity_reconstruction_rhs =
          g_.template get_varying<nd_dtype::real, N>(
              "pt16_velocity_reconstruction_rhs");
      position = g_.template get_varying<nd_dtype::real, N>("position");
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

  using real = dtype_t<nd_dtype::real>;

  std::vector<real> cg_r, cg_y, cg_p, cg_x, cg_Ap;

  void cg_resize(size_t size) {
    cg_r.resize(size);
    cg_y.resize(size);
    cg_p.resize(size);
    cg_x.resize(size);
    cg_Ap.resize(size);
  }

private:
  template <
      typename NHood_, typename Group_, typename IterateF_, typename ProductF_,
      typename RHSF_, typename DiagonalF_>
  size_t solve_cg_dp(
      NHood_ const &nhood_, Group_ &p, IterateF_ iterate, ProductF_ product,
      RHSF_ rhs, DiagonalF_ diagonal, real const tol = 1e-2,
      size_t const max_iterations = 50) {
    // alias for the math_policy member types
    using l = typename math_policy::literals;
    using c = typename math_policy::constants;
    using o = typename math_policy::operations;

    // {{{
    auto _parallel = [this](auto f) {
      _Pragma("omp parallel") {
        _Pragma("omp single") {
          auto const thread_count = static_cast<size_t>(omp_get_num_threads());
          _per_thread.resize(thread_count);
        } // pragma omp single

        auto const thread_index = static_cast<size_t>(omp_get_thread_num());

        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

        // select and resize the neighbor storage for the current thread
        auto &neighbors = _per_thread[thread_index].neighbors;
        neighbors.resize(_group_count);

        for (auto &pgn : neighbors)
          pgn.reserve(100);

        f(thread_index);
      }
    };

    auto _find_neighbors =
        [ this, &nhood_ ](auto tid, auto &p, size_t i) -> auto & {
      auto &neighbors = _per_thread[tid].neighbors;

      // clean up the neighbor storage
      for (auto &pgn : neighbors)
        pgn.clear();

      // find all neighbors of (p, i)
      nhood_.neighbors(p._index, i, [&neighbors](auto n_index, auto j) {
        neighbors[n_index].push_back(j);
      });

      return neighbors;
    };

    auto _foreach_particle = [this, &nhood_](auto tid, auto &p, auto f) {
#pragma omp for
      for (size_t i = 0; i < p._count; ++i) {
        f(tid, i);
      }
    };

    auto _parallel_foreach_particle_in_group = [&, this](auto &p, auto f) {
      if (p._count == 0)
        return;

      _parallel([&, this](auto tid) {
        _foreach_particle(tid, p, [&, this](auto tid, size_t i) {
          // call the per-particle function
          f(tid, i);
        });
      });
    };
    // }}}

    cg_resize(p._count);

    // r_{0} = A * x_{0} - b
    // y_{0} = M^{-1} * r_{0}
    // p_{0} = - y_{0}
    _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
      auto &neighbors = _find_neighbors(tid, p, i);
      // bound _iterate for use in _product
      auto _x = [&](size_t i) { return iterate(p, i, neighbors); };
      // compute row-by-row
      cg_r[i] = product(p, i, neighbors, _x) - rhs(p, i, neighbors);
      cg_y[i] = cg_r[i] / diagonal(p, i, neighbors);
      cg_p[i] = -cg_y[i];
    });

    real r_norm_squared = tol + 1;

    size_t iterations = 0;
    for (; iterations < max_iterations and r_norm_squared > tol; ++iterations) {
      r_norm_squared = 0;

      // alpha = r_k^T y_k / ( p_k^T A p_k )
      real alpha_nom = 0, alpha_den = 0;
      _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
        auto &neighbors = _find_neighbors(tid, p, i);
        cg_Ap[i] = product(
            p, i, neighbors, [&cg_p = cg_p](size_t i) { return cg_p[i]; });
#pragma omp atomic
        alpha_nom += cg_r[i] * cg_y[i];
#pragma omp atomic
        alpha_den += cg_p[i] * cg_Ap[i];
#pragma omp atomic
        r_norm_squared += cg_r[i] * cg_r[i];
      });
      real const alpha = alpha_nom / alpha_den;

      if (std::sqrt(r_norm_squared) < tol)
        break;

      // x_{k+1} = x_k + alpha * p_k
      // r_{k+1} = r_k + alpha * A * p_k
      // y_{k+1} = M^{-1} * r_{k+1}
      _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
        auto &neighbors = _find_neighbors(tid, p, i);
        // bind _iterate for use in _product
        auto _x = [&](size_t i) -> auto & { return iterate(p, i, neighbors); };
        // compute row-by-row
        _x(i) += alpha * cg_p[i];
        cg_r[i] += alpha * cg_Ap[i];
        cg_y[i] = cg_r[i] / diagonal(p, i, neighbors);
      });

      // beta = r_{k+1}^T y_{k+1} / ( r_{k}^T y_{k} )
      real beta_nom = 0;
      _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
#pragma omp atomic
        beta_nom += cg_r[i] * cg_y[i];
      });
      real const beta = beta_nom / alpha_nom;

      // p_{k+1} = - y_{k+1} + beta * p_{k}
      _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
        auto &neighbors = _find_neighbors(tid, p, i);
        // compute row-by-row
        cg_p[i] = -cg_y[i] + cg_p[i];
      });
    }

    return iterations;
  }

public:
  template <typename NHood_> size_t vorticity_diffusion(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member types
    using l = typename math_policy::literals;
    using c = typename math_policy::constants;
    using o = typename math_policy::operations;

    // {{{
    auto _parallel = [this](auto f) {
      _Pragma("omp parallel") {
        _Pragma("omp single") {
          auto const thread_count = static_cast<size_t>(omp_get_num_threads());
          _per_thread.resize(thread_count);
        } // pragma omp single

        auto const thread_index = static_cast<size_t>(omp_get_thread_num());

        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

        // select and resize the neighbor storage for the current thread
        auto &neighbors = _per_thread[thread_index].neighbors;
        neighbors.resize(_group_count);

        for (auto &pgn : neighbors)
          pgn.reserve(100);

        f(thread_index);
      }
    };

    auto _find_neighbors =
        [ this, &nhood_ ](auto tid, auto &p, size_t i) -> auto & {
      auto &neighbors = _per_thread[tid].neighbors;

      // clean up the neighbor storage
      for (auto &pgn : neighbors)
        pgn.clear();

      // find all neighbors of (p, i)
      nhood_.neighbors(p._index, i, [&neighbors](auto n_index, auto j) {
        neighbors[n_index].push_back(j);
      });

      return neighbors;
    };

    auto _foreach_particle = [this, &nhood_](auto tid, auto &p, auto f) {
#pragma omp for
      for (size_t i = 0; i < p._count; ++i) {
        f(tid, i);
      }
    };

    auto _parallel_foreach_particle_in_group = [&, this](auto &p, auto f) {
      if (p._count == 0)
        return;

      _parallel([&, this](auto tid) {
        _foreach_particle(tid, p, [&, this](auto tid, size_t i) {
          // call the per-particle function
          f(tid, i);
        });
      });
    };
    // }}}

    size_t d = 0;

    auto _diagonal = [this, &g](auto &p, size_t f, auto &) {
      return p.pt16_vorticity_diffusion_diagonal[f];
    };

    auto _product = [this, d, &g,
                     &_diagonal](auto &p, size_t f, auto &neighbors, auto x) {
      auto const h = g.smoothing_scale[0];

      real result = _diagonal(p, f, neighbors) * x(f);

      for (auto f_f : neighbors[p._index]) {
        if (f != f_f)
          result -= p.mass[f_f] *
                    o::kernel_h(p.position[f] - p.position[f_f], h) * x(f_f);
      }

      return result;
    };

    auto _rhs = [this, d](auto &p, size_t f, auto &) {
      return p.pt16_vorticity_diffusion_rhs[f][d];
    };

    auto _iterate = [ this, d ](auto &p, size_t f, auto &) -> auto & {
      return p.vorticity[f][d];
    };

    real const tol = 1e-2;
    size_t const max_iterations = 50;

    size_t iterations = 0;

    for (auto &p : _data.by_group_type.fluid) {
      cg_resize(p._count);

      // r_{0} = A * x_{0} - b
      // y_{0} = M^{-1} * r_{0}
      // p_{0} = - y_{0}
      _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
        auto &neighbors = _find_neighbors(tid, p, i);
        // bound _iterate for use in _product
        auto _x = [&, this](size_t i) { return _iterate(p, i, neighbors); };
        // compute row-by-row
        cg_r[i] = _product(p, i, neighbors, _x) - _rhs(p, i, neighbors);
        cg_y[i] = cg_r[i] / _diagonal(p, i, neighbors);
        cg_p[i] = -cg_y[i];
      });

      real r_norm_squared = tol + 1;

      for (; iterations < max_iterations and r_norm_squared > tol;
           ++iterations) {
        r_norm_squared = 0;

        // alpha = r_k^T y_k / ( p_k^T A p_k )
        real alpha_nom = 0, alpha_den = 0;
        _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
          auto &neighbors = _find_neighbors(tid, p, i);
          cg_Ap[i] = _product(
              p, i, neighbors, [&cg_p = cg_p](size_t i) { return cg_p[i]; });
#pragma omp atomic
          alpha_nom += cg_r[i] * cg_y[i];
#pragma omp atomic
          alpha_den += cg_p[i] * cg_Ap[i];
#pragma omp atomic
          r_norm_squared += cg_r[i] * cg_r[i];
        });
        real const alpha = alpha_nom / alpha_den;

        if (std::sqrt(r_norm_squared) < tol)
          break;

        // x_{k+1} = x_k + alpha * p_k
        // r_{k+1} = r_k + alpha * A * p_k
        // y_{k+1} = M^{-1} * r_{k+1}
        _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
          auto &neighbors = _find_neighbors(tid, p, i);
          // bind _iterate for use in _product
          auto _x = [&, this ](size_t i) -> auto & {
            return _iterate(p, i, neighbors);
          };
          // compute row-by-row
          _x(i) += alpha * cg_p[i];
          cg_r[i] += alpha * cg_Ap[i];
          cg_y[i] = cg_r[i] / _diagonal(p, i, neighbors);
        });

        // beta = r_{k+1}^T y_{k+1} / ( r_{k}^T y_{k} )
        real beta_nom = 0;
        _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
#pragma omp atomic
          beta_nom += cg_r[i] * cg_y[i];
        });
        real const beta = beta_nom / alpha_nom;

        // p_{k+1} = - y_{k+1} + beta * p_{k}
        _parallel_foreach_particle_in_group(p, [&, this](auto tid, size_t i) {
          auto &neighbors = _find_neighbors(tid, p, i);
          // compute row-by-row
          cg_p[i] = -cg_y[i] + cg_p[i];
        });
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
