#pragma once

#include "scheme_base.hpp"

#include <prtcl/data/component_type.hpp>
#include <prtcl/data/group.hpp>
#include <prtcl/data/model.hpp>
#include <prtcl/data/uniform_field.hpp>
#include <prtcl/data/varying_field.hpp>

#include <prtcl/math.hpp>
#include <prtcl/math/aat13.hpp>
#include <prtcl/math/kernel.hpp>

#include <prtcl/solver/cg_openmp.hpp>

#include <prtcl/log.hpp>

#include <prtcl/util/neighborhood.hpp>

#include <sstream>
#include <string_view>
#include <vector>

#include <omp.h>

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#define PRTCL_RT_LOG_TRACE_SCOPED(...)
namespace prtcl {
namespace schemes {

template <typename T, size_t N, template <typename, size_t> typename K>
class pt16 : public SchemeBase {
public:
  using real = T;
  using integer = int32_t;
  using boolean = bool;

  using kernel_type = K<T, N>;

  template <typename U, size_t... M>
  using Tensor = math::Tensor<U, M...>;

private:
  struct global_data {
    UniformFieldSpan<real> g_h;
    UniformFieldSpan<real> g_dt;
    UniformFieldSpan<real> g_t;
    UniformFieldSpan<real> g_dt_fade;
  };

private:
  struct groups_fluid_data {
    size_t _count;
    size_t index;

    // uniform fields
    UniformFieldSpan<real> u_rho0;
    UniformFieldSpan<real> u_xi;
    UniformFieldSpan<real> u_pcg_max_error;
    UniformFieldSpan<integer> u_pcg_max_iters;
    UniformFieldSpan<integer> u_pcg_iterations;

    // varying fields
    VaryingFieldSpan<real, N> v_x;
    VaryingFieldSpan<real, N> v_v;
    VaryingFieldSpan<real, N> v_a;
    VaryingFieldSpan<real> v_rho;
    VaryingFieldSpan<real> v_m;
    VaryingFieldSpan<real, N, N> v_L;
    VaryingFieldSpan<real> v_t_birth;
    VaryingFieldSpan<real> v_pt16_one;
    VaryingFieldSpan<real> v_pt16_rho;
    VaryingFieldSpan<real> v_pt16_same_rho;
    VaryingFieldSpan<real, N> v_omega;
    VaryingFieldSpan<real, N, N> v_tvg;
    VaryingFieldSpan<real> v_omega_diagonal;
    VaryingFieldSpan<real, N> v_omega_rhs;
    VaryingFieldSpan<real> v_v_rec_diagonal;
    VaryingFieldSpan<real, N> v_v_rec_rhs;

    static bool selects(Group const &group) {
      return (group.GetGroupType() == "fluid");
    }
  };

private:
  struct groups_boundary_data {
    size_t _count;
    size_t index;

    // uniform fields

    // varying fields
    VaryingFieldSpan<real, N> v_x;
    VaryingFieldSpan<real> v_V;

    static bool selects(Group const &group) {
      return (group.GetGroupType() == "boundary");
    }
  };

private:
  struct {
    global_data global;

    struct {
      std::vector<groups_fluid_data> fluid;
      std::vector<groups_boundary_data> boundary;
    } groups;

    size_t group_count;
  } _data;

private:
  struct per_thread_data {
    std::vector<std::vector<size_t>> neighbors;
  };

  std::vector<per_thread_data> _per_thread;

public:
  pt16() {
    this->RegisterProcedure("setup", &pt16::setup);
    this->RegisterProcedure(
        "solve_vorticity_diffusion", &pt16::solve_vorticity_diffusion);
    this->RegisterProcedure(
        "vorticity_preservation", &pt16::vorticity_preservation);
    this->RegisterProcedure(
        "solve_velocity_reconstruction", &pt16::solve_velocity_reconstruction);
  }

public:
  std::string GetFullName() const override { return GetFullNameImpl(); }

public:
  void Load(Model &model) final {
    // global fields
    _data.global.g_h = model.AddGlobalFieldImpl<real>("smoothing_scale");
    _data.global.g_dt = model.AddGlobalFieldImpl<real>("time_step");
    _data.global.g_t = model.AddGlobalFieldImpl<real>("current_time");
    _data.global.g_dt_fade = model.AddGlobalFieldImpl<real>("fade_duration");

    auto group_count = model.GetGroupCount();
    _data.group_count = group_count;

    _data.groups.fluid.clear();
    _data.groups.boundary.clear();

    for (size_t group_index = 0; group_index < group_count; ++group_index) {
      auto &group = model.GetGroups()[group_index];

      if (groups_fluid_data::selects(group)) {
        auto &data = _data.groups.fluid.emplace_back();

        data._count = group.GetItemCount();
        data.index = group_index;

        // uniform fields
        data.u_rho0 = group.AddUniformFieldImpl<real>("rest_density");
        data.u_xi = group.AddUniformFieldImpl<real>("strain_rate_viscosity");
        data.u_pcg_max_error =
            group.AddUniformFieldImpl<real>("pt16_maximum_error");
        data.u_pcg_max_iters =
            group.AddUniformFieldImpl<integer>("pt16_maximum_iterations");
        data.u_pcg_iterations =
            group.AddUniformFieldImpl<integer>("pt16_iterations");

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_v = group.AddVaryingFieldImpl<real, N>("velocity");
        data.v_a = group.AddVaryingFieldImpl<real, N>("acceleration");
        data.v_rho = group.AddVaryingFieldImpl<real>("density");
        data.v_m = group.AddVaryingFieldImpl<real>("mass");
        data.v_L = group.AddVaryingFieldImpl<real, N, N>("gradient_correction");
        data.v_t_birth = group.AddVaryingFieldImpl<real>("time_of_birth");
        data.v_pt16_one = group.AddVaryingFieldImpl<real>("pt16_normalizer");
        data.v_pt16_rho = group.AddVaryingFieldImpl<real>("pt16_density");
        data.v_pt16_same_rho =
            group.AddVaryingFieldImpl<real>("pt16_same_density");
        data.v_omega = group.AddVaryingFieldImpl<real, N>("vorticity");
        data.v_tvg =
            group.AddVaryingFieldImpl<real, N, N>("target_velocity_gradient");
        data.v_omega_diagonal = group.AddVaryingFieldImpl<real>(
            "pt16_vorticity_diffusion_diagonal");
        data.v_omega_rhs =
            group.AddVaryingFieldImpl<real, N>("pt16_vorticity_diffusion_rhs");
        data.v_v_rec_diagonal = group.AddVaryingFieldImpl<real>(
            "pt16_velocity_reconstruction_diagonal");
        data.v_v_rec_rhs = group.AddVaryingFieldImpl<real, N>(
            "pt16_velocity_reconstruction_rhs");
      }

      if (groups_boundary_data::selects(group)) {
        auto &data = _data.groups.boundary.emplace_back();

        data._count = group.GetItemCount();
        data.index = group_index;

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_V = group.AddVaryingFieldImpl<real>("volume");
      }
    }
  }

public:
  void setup(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    {// foreach fluid particle f
#pragma omp parallel
     {PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

    auto &t = _per_thread[omp_get_thread_num()];

    // select, resize and reserve neighbor storage
    auto &neighbors = t.neighbors;
    neighbors.resize(_data.group_count);
    for (auto &pgn : neighbors)
      pgn.reserve(100);

    for (auto &p : _data.groups.fluid) {
#pragma omp for
      for (size_t i = 0; i < p._count; ++i) {
        // cleanup neighbor storage
        for (auto &pgn : neighbors)
          pgn.clear();

        // find all neighbors of (p, i)
        nhood.CopyNeighbors(p.index, i, neighbors);

        // compute
        p.v_pt16_same_rho[i] = static_cast<T>(0);

        { // foreach @particle@ neighbor f_f
          auto &n = p;

          for (auto const j : neighbors[n.index]) {
            // compute
            p.v_pt16_same_rho[i] +=
                (n.v_m[j] *
                 o::kernel_h<kernel_type>((p.v_x[i] - n.v_x[j]), *g.g_h));
          }
        } // foreach @particle@ neighbor f_f

        // compute
        p.v_pt16_rho[i] = static_cast<T>(0);

        { // foreach fluid neighbor f_f
          for (auto &n : _data.groups.fluid) {
            for (auto const j : neighbors[n.index]) {
              // compute
              p.v_pt16_rho[i] +=
                  ((n.v_m[j] / *n.u_rho0) *
                   o::kernel_h<kernel_type>((p.v_x[i] - n.v_x[j]), *g.g_h));
            }
          }
        } // foreach fluid neighbor f_f

        { // foreach boundary neighbor f_b
          for (auto &n : _data.groups.boundary) {
            for (auto const j : neighbors[n.index]) {
              // compute
              p.v_pt16_rho[i] +=
                  (n.v_V[j] *
                   o::kernel_h<kernel_type>((p.v_x[i] - n.v_x[j]), *g.g_h));
            }
          }
        } // foreach boundary neighbor f_b

        // compute
        p.v_pt16_rho[i] *= *p.u_rho0;
      }
    }
  } // omp parallel region
} // foreach fluid particle f

{ // foreach fluid particle f
#pragma omp parallel
  {
    PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

    auto &t = _per_thread[omp_get_thread_num()];

    // select, resize and reserve neighbor storage
    auto &neighbors = t.neighbors;
    neighbors.resize(_data.group_count);
    for (auto &pgn : neighbors)
      pgn.reserve(100);

    for (auto &p : _data.groups.fluid) {
#pragma omp for
      for (size_t i = 0; i < p._count; ++i) {
        // cleanup neighbor storage
        for (auto &pgn : neighbors)
          pgn.clear();

        // find all neighbors of (p, i)
        nhood.CopyNeighbors(p.index, i, neighbors);

        // local_def ...;
        Tensor<real, N, N> l_vg_f = o::template zeros<real, N, N>();

        { // foreach @particle@ neighbor f_f
          auto &n = p;

          for (auto const j : neighbors[n.index]) {
            // compute
            l_vg_f +=
                (n.v_m[j] * o::outer_product(
                                (n.v_v[j] - p.v_v[i]),
                                o::kernel_gradient_h<kernel_type>(
                                    (p.v_x[i] - n.v_x[j]), *g.g_h)));
          }
        } // foreach @particle@ neighbor f_f

        // compute
        l_vg_f /= p.v_rho[i];

        // local_def ...;
        Tensor<real> l_divergence_f = o::trace(l_vg_f);

        // local_def ...;
        Tensor<real, N, N> l_R_f =
            ((l_vg_f - o::transpose(l_vg_f)) / static_cast<T>(2));

        // local_def ...;
        Tensor<real, N, N> l_V_f =
            ((l_divergence_f / static_cast<T>(3)) *
             o::template identity<real, N, N>());

        // local_def ...;
        Tensor<real, N, N> l_S_f =
            (((l_vg_f + o::transpose(l_vg_f)) / static_cast<T>(2)) - l_V_f);

        // compute
        p.v_omega[i] =
            (static_cast<T>(2) * o::vector_from_cross_product_matrix(l_R_f));

        // compute
        p.v_tvg[i] =
            ((l_V_f * o::unit_step_r(
                          static_cast<T>(0), (p.v_rho[i] - *p.u_rho0),
                          (-l_divergence_f))) +
             (*p.u_xi * l_S_f));
      }
    }
  } // omp parallel region
} // foreach fluid particle f
} // namespace schemes

public:
void solve_vorticity_diffusion(Neighborhood const &nhood) {
  auto &g = _data.global;

  // mathematical operations
  namespace o = ::prtcl::math;

  // resize per-thread storage
  _per_thread.resize(omp_get_max_threads());

  { // foreach fluid particle f
#pragma omp parallel
    {
      PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

      auto &t = _per_thread[omp_get_thread_num()];

      // select, resize and reserve neighbor storage
      auto &neighbors = t.neighbors;
      neighbors.resize(_data.group_count);
      for (auto &pgn : neighbors)
        pgn.reserve(100);

      for (auto &p : _data.groups.fluid) {
#pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          // cleanup neighbor storage
          for (auto &pgn : neighbors)
            pgn.clear();

          // find all neighbors of (p, i)
          nhood.CopyNeighbors(p.index, i, neighbors);

          // compute
          p.v_omega_rhs[i] = o::template zeros<real, N>();

          { // foreach @particle@ neighbor f_f
            auto &n = p;

            for (auto const j : neighbors[n.index]) {
              // compute
              p.v_omega_rhs[i] +=
                  ((n.v_m[j] * (p.v_omega[i] - n.v_omega[j])) *
                   o::kernel_h<kernel_type>((p.v_x[i] - n.v_x[j]), *g.g_h));
            }
          } // foreach @particle@ neighbor f_f

          // compute
          p.v_omega_rhs[i] *= *p.u_xi;
        }
      }
    } // omp parallel region
  }   // foreach fluid particle f

  // foreach dimension index dim
  for (size_t i_dim = 0; i_dim < N; ++i_dim) {
    { // solve_pcg ...
      // the solver object
      static CGOpenMP<T> solver;

      // iterate over the per-thread storage
      for (auto &t : _per_thread) {
        // select, resize and reserve neighbor storage
        auto &neighbors = t.neighbors;
        neighbors.resize(_data.group_count);
        for (auto &pgn : neighbors)
          pgn.reserve(100);
      }

      for (auto &p : _data.groups.fluid) {
        auto setup_r = [&](auto &p, size_t i, auto &p_into) {
          // solve_setup

          // compute
          p_into[i] = (p.v_omega_rhs[i])[i_dim];
        };

        auto setup_g = [&](auto &p, size_t i, auto &p_into) {
          // solve_setup

          // compute
          p_into[i] = (p.v_omega[i])[i_dim];
        };

        auto product_s = [&](auto &p, size_t i, auto &p_into, auto &p_with) {
          // solve_product

          // fetch the neighbor storage for this thread
          auto &neighbors = _per_thread[omp_get_thread_num()].neighbors;

          // cleanup neighbor storage
          for (auto &pgn : neighbors)
            pgn.clear();

          // find all neighbors of (p, i)
          nhood.CopyNeighbors(p.index, i, neighbors);

          // compute
          p_into[i] = (p.v_pt16_same_rho[i] * p_with[i]);

          { // foreach @particle@ neighbor f_f
            auto &n = p;

            for (auto const j : neighbors[n.index]) {
              // compute
              p_into[i] -=
                  ((n.v_m[j] *
                    o::kernel_h<kernel_type>((p.v_x[i] - n.v_x[j]), *g.g_h)) *
                   p_with[j]);
            }
          } // foreach @particle@ neighbor f_f
        };

        auto product_p = [&](auto &p, size_t i, auto &p_into, auto &p_with) {
          // solve_product

          // compute
          p_into[i] =
              (p_with[i] /
               (p.v_pt16_same_rho[i] -
                (p.v_m[i] * o::kernel_h<kernel_type>(
                                o::template zeros<real, N>(), *g.g_h))));
        };

        auto apply = [&](auto &p, size_t i, auto &p_with) {
          // solve_apply

          // compute
          (p.v_omega[i])[i_dim] =
              ((p_with[i] * o::unit_step_l(
                                static_cast<T>(0),
                                ((*g.g_t - p.v_t_birth[i]) - *g.g_dt_fade))) +
               ((p.v_omega[i])[i_dim] *
                o::unit_step_r(
                    static_cast<T>(0),
                    (*g.g_dt_fade - (*g.g_t - p.v_t_birth[i])))));
        };

        auto const iterations = solver.solve(
            p, setup_r, setup_g, product_s, product_p, apply,
            *p.u_pcg_max_error, *p.u_pcg_max_iters);

        *p.u_pcg_iterations = iterations;

        log::Debug("scheme", "scheme", "pcg solver iterations ", iterations);
      }

    } // ... solve_pcg
  }   // foreach dimension index dim

  { // foreach fluid particle f
#pragma omp parallel
    {
      PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

      auto &t = _per_thread[omp_get_thread_num()];

      for (auto &p : _data.groups.fluid) {
#pragma omp for schedule(static)
        for (size_t i = 0; i < p._count; ++i) {
          // compute
          p.v_tvg[i] += o::cross_product_matrix_from_vector(
              (static_cast<T>(0.5) * p.v_omega[i]));
        }
      }
    } // omp parallel region
  }   // foreach fluid particle f
}

public:
void vorticity_preservation(Neighborhood const &nhood) {
  auto &g = _data.global;

  // mathematical operations
  namespace o = ::prtcl::math;

  // resize per-thread storage
  _per_thread.resize(omp_get_max_threads());

  { // foreach fluid particle f
#pragma omp parallel
    {
      PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

      auto &t = _per_thread[omp_get_thread_num()];

      for (auto &p : _data.groups.fluid) {
#pragma omp for schedule(static)
        for (size_t i = 0; i < p._count; ++i) {
          // compute
          p.v_tvg[i] += o::cross_product_matrix_from_vector(
              (static_cast<T>(0.5) * p.v_omega[i]));
        }
      }
    } // omp parallel region
  }   // foreach fluid particle f
}

public:
void solve_velocity_reconstruction(Neighborhood const &nhood) {
  auto &g = _data.global;

  // mathematical operations
  namespace o = ::prtcl::math;

  // resize per-thread storage
  _per_thread.resize(omp_get_max_threads());

  { // foreach fluid particle f
#pragma omp parallel
    {
      PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

      auto &t = _per_thread[omp_get_thread_num()];

      // select, resize and reserve neighbor storage
      auto &neighbors = t.neighbors;
      neighbors.resize(_data.group_count);
      for (auto &pgn : neighbors)
        pgn.reserve(100);

      for (auto &p : _data.groups.fluid) {
#pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          // cleanup neighbor storage
          for (auto &pgn : neighbors)
            pgn.clear();

          // find all neighbors of (p, i)
          nhood.CopyNeighbors(p.index, i, neighbors);

          // compute
          p.v_v_rec_rhs[i] = o::template zeros<real, N>();

          { // foreach @particle@ neighbor f_f
            auto &n = p;

            for (auto const j : neighbors[n.index]) {
              // compute
              p.v_v_rec_rhs[i] +=
                  (((n.v_m[j] * static_cast<T>(0.5)) *
                    ((p.v_tvg[i] + n.v_tvg[j]) * (p.v_x[i] - n.v_x[j]))) *
                   o::kernel_h<kernel_type>((p.v_x[i] - n.v_x[j]), *g.g_h));
            }
          } // foreach @particle@ neighbor f_f
        }
      }
    } // omp parallel region
  }   // foreach fluid particle f

  // foreach dimension index dim
  for (size_t i_dim = 0; i_dim < N; ++i_dim) {
    { // solve_pcg ...
      // the solver object
      static CGOpenMP<T> solver;

      // iterate over the per-thread storage
      for (auto &t : _per_thread) {
        // select, resize and reserve neighbor storage
        auto &neighbors = t.neighbors;
        neighbors.resize(_data.group_count);
        for (auto &pgn : neighbors)
          pgn.reserve(100);
      }

      for (auto &p : _data.groups.fluid) {
        auto setup_r = [&](auto &p, size_t i, auto &p_into) {
          // solve_setup

          // compute
          p_into[i] = (p.v_v_rec_rhs[i])[i_dim];
        };

        auto setup_g = [&](auto &p, size_t i, auto &p_into) {
          // solve_setup

          // compute
          p_into[i] = (p.v_v[i])[i_dim];
        };

        auto product_s = [&](auto &p, size_t i, auto &p_into, auto &p_with) {
          // solve_product

          // fetch the neighbor storage for this thread
          auto &neighbors = _per_thread[omp_get_thread_num()].neighbors;

          // cleanup neighbor storage
          for (auto &pgn : neighbors)
            pgn.clear();

          // find all neighbors of (p, i)
          nhood.CopyNeighbors(p.index, i, neighbors);

          // compute
          p_into[i] = (p.v_pt16_rho[i] * p_with[i]);

          { // foreach @particle@ neighbor f_f
            auto &n = p;

            for (auto const j : neighbors[n.index]) {
              // compute
              p_into[i] -=
                  ((n.v_m[j] *
                    o::kernel_h<kernel_type>((p.v_x[i] - n.v_x[j]), *g.g_h)) *
                   p_with[j]);
            }
          } // foreach @particle@ neighbor f_f
        };

        auto product_p = [&](auto &p, size_t i, auto &p_into, auto &p_with) {
          // solve_product

          // compute
          p_into[i] =
              (p_with[i] /
               (p.v_pt16_same_rho[i] -
                (p.v_m[i] * o::kernel_h<kernel_type>(
                                o::template zeros<real, N>(), *g.g_h))));
        };

        auto apply = [&](auto &p, size_t i, auto &p_with) {
          // solve_apply

          // compute
          (p.v_v[i])[i_dim] =
              ((p_with[i] * o::unit_step_l(
                                static_cast<T>(0),
                                ((*g.g_t - p.v_t_birth[i]) - *g.g_dt_fade))) +
               ((p.v_v[i])[i_dim] *
                o::unit_step_r(
                    static_cast<T>(0),
                    (*g.g_dt_fade - (*g.g_t - p.v_t_birth[i])))));
        };

        auto const iterations = solver.solve(
            p, setup_r, setup_g, product_s, product_p, apply,
            *p.u_pcg_max_error, *p.u_pcg_max_iters);

        *p.u_pcg_iterations = iterations;

        log::Debug("scheme", "scheme", "pcg solver iterations ", iterations);
      }

    } // ... solve_pcg
  }   // foreach dimension index dim
}

private:
static std::string GetFullNameImpl() {
  std::ostringstream ss;
  ss << "prtcl::schemes::pt16";
  ss << "[T=" << MakeComponentType<T>() << ", N=" << N
     << ", K=" << kernel_type::get_name() << "]";
  return ss.str();
}

public:
std::string_view GetPrtclSourceCode() const final {
  return R"prtcl(

scheme pt16 {
  groups fluid {
    select type fluid;

    varying field x = real[] position;
    varying field v = real[] velocity;
    varying field a = real[] acceleration;

    varying field rho = real density;
    varying field m = real mass;

    varying field L = real[][] gradient_correction;

    uniform field rho0 = real rest_density;

    varying field t_birth = real time_of_birth;

    varying field pt16_one = real pt16_normalizer;
    varying field pt16_rho = real pt16_density;
    varying field pt16_same_rho = real pt16_same_density;

    varying field omega = real[] vorticity;

    varying field tvg = real[][] target_velocity_gradient;

    varying field omega_diagonal = real   pt16_vorticity_diffusion_diagonal;
    varying field omega_rhs      = real[] pt16_vorticity_diffusion_rhs;

    varying field v_rec_diagonal = real   pt16_velocity_reconstruction_diagonal;
    varying field v_rec_rhs      = real[] pt16_velocity_reconstruction_rhs;

    uniform field xi = real strain_rate_viscosity;

    uniform field pcg_max_error  = real    pt16_maximum_error;
    uniform field pcg_max_iters  = integer pt16_maximum_iterations;
    uniform field pcg_iterations = integer pt16_iterations;
  }

  groups boundary {
    select type boundary;

    varying field x = real[] position;
    varying field V = real volume;
  }

  global {
    field h = real smoothing_scale;
    field dt = real time_step;

    field t = real current_time;
    field dt_fade = real fade_duration;
  }

  procedure setup {
    foreach fluid particle f {
      // same-group density
      compute pt16_same_rho.f = 0;

      foreach _ neighbor f_f {
        compute pt16_same_rho.f += m.f_f * kernel_h(x.f - x.f_f, h);
      }

      // smoothed density field (from SPlisHSPlasH / Viscosity / Peer16)
      compute pt16_rho.f = 0;

      foreach fluid neighbor f_f {
        compute pt16_rho.f += (m.f_f / rho0.f_f) * kernel_h(x.f - x.f_f, h);
      }
      
      foreach boundary neighbor f_b {
        compute pt16_rho.f += V.f_b * kernel_h(x.f - x.f_b, h);
      }
      
      compute pt16_rho.f *= rho0.f;
    }

    foreach fluid particle f {
      // initialize the velocity gradient (matrix)
      local vg_f : real[][] = zeros<real[][]>();

      // accumulate the velocity gradient only from particles of the same group
      foreach _ neighbor f_f {
        compute vg_f +=
          m.f_f
        *
          outer_product(
            v.f_f - v.f,
            kernel_gradient_h(x.f - x.f_f, h)
          );
      }

      compute vg_f /= rho.f; // pt16_same_rho.f;

      // decompose the velocity gradient into spin, expansion and shear rate
      local divergence_f : real = trace(vg_f);
      local R_f : real[][] = (vg_f - transpose(vg_f)) / 2;
      local V_f : real[][] = (divergence_f / 3) * identity<real[][]>();
      local S_f : real[][] = (vg_f + transpose(vg_f)) / 2 - V_f;

      // extract the vorticity from the spin rate
      compute omega.f = 2 * vector_from_cross_product_matrix(R_f);

      // partially compose the target velocity gradient (without spin rate)
      compute tvg.f =
          V_f * unit_step_r(0, rho.f - rho0.f, -divergence_f)
          // TODO: is divergence_f >= 0 or -divergence_f >= 0 correct?
        +
          xi.f * S_f;
    }
  }

  procedure solve_vorticity_diffusion {
    // compute the right-hand-side of the vorticity diffusion system
    foreach fluid particle f {
      compute omega_rhs.f = zeros<real[]>();

      foreach _ neighbor f_f {
        compute omega_rhs.f +=
            m.f_f
          *
            (omega.f - omega.f_f)
          *
            kernel_h(x.f - x.f_f, h);
      }

      compute omega_rhs.f *= xi.f;
    }

    foreach dimension index dim {
      solve pcg real over fluid particle f {
        setup right_hand_side into result {
          compute result.f = omega_rhs.f[dim];
        }

        setup guess into iterate {
          compute iterate.f = omega.f[dim];
        }

        product preconditioner with iterate into result {
          compute result.f =
              iterate.f
            /
              (pt16_same_rho.f - m.f * kernel_h(zeros<real[]>(), h));
        }

        product system with iterate into result {
          compute result.f = pt16_same_rho.f * iterate.f;
          
          foreach _ neighbor f_f {
            compute result.f -= m.f_f * kernel_h(x.f - x.f_f, h) * iterate.f_f;
          }
        }

        apply iterate {
          // finalize the target vorticity
          compute omega.f[dim] =
              iterate.f * unit_step_l(0, (t - t_birth.f) - dt_fade)
            +
              omega.f[dim] * unit_step_r(0, dt_fade - (t - t_birth.f));
        }
      }
    }
    
    // finalize target velocity gradient
    foreach fluid particle f {
      // add the target spin rate (from the vorticity) to the velocity gradient
      compute tvg.f += cross_product_matrix_from_vector(0.5 * omega.f);
    }
  }

  procedure vorticity_preservation {
    foreach fluid particle f {
      // add the target spin rate (from the vorticity) to the velocity gradient
      compute tvg.f += cross_product_matrix_from_vector(0.5 * omega.f);
    }
  }

  procedure solve_velocity_reconstruction {
    foreach fluid particle f {
      compute v_rec_rhs.f = zeros<real[]>();
      
      foreach _ neighbor f_f {
        compute v_rec_rhs.f +=
            m.f_f
          *
            0.5 * ((tvg.f + tvg.f_f) * (x.f - x.f_f))
          *
            kernel_h(x.f - x.f_f, h);
      }
    }
  
    foreach dimension index dim {
      solve pcg real over fluid particle f {
        setup right_hand_side into result {
          compute result.f = v_rec_rhs.f[dim];
        }

        setup guess into iterate {
          compute iterate.f = v.f[dim];
        }

        product preconditioner with iterate into result {
          compute result.f =
              iterate.f
            /
              (pt16_same_rho.f - m.f * kernel_h(zeros<real[]>(), h));
        }

        product system with iterate into result {
          compute result.f = pt16_rho.f * iterate.f;

          foreach _ neighbor f_f {
            compute result.f -= m.f_f * kernel_h(x.f - x.f_f, h) * iterate.f_f;
          }
        }

        apply iterate {
          // implement fade-in of particles by applying the resulting iterate
          // after the fade was completed and the previous velocity if the
          // particle is still fading in
          compute v.f[dim] =
              iterate.f * unit_step_l(0, (t - t_birth.f) - dt_fade)
            +
              v.f[dim] * unit_step_r(0, dt_fade - (t - t_birth.f));

          //compute v.f[dim] = iterate.f;
          
          //compute v.f = iterate.f * unit_step_l(0, (t - t_birth.f) - dt_fade);

          // compute the viscosity acceleration
          //compute a.f[dim] = (iterate.f - v.f[dim]) / dt; // NOTE NEGATED
        }
      }
    }
  }
}

)prtcl";
}

private:
static inline bool const registered_ =
    GetSchemeRegistry().RegisterScheme<pt16>(GetFullNameImpl());

friend void Register_pt16();
}; // namespace prtcl
}
}
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
