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
class aat13 : public SchemeBase {
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
  };

private:
  struct groups_fluid_data {
    size_t _count;
    size_t index;

    // uniform fields
    UniformFieldSpan<real> u_rho0;
    UniformFieldSpan<real> u_gamma;

    // varying fields
    VaryingFieldSpan<real, N> v_x;
    VaryingFieldSpan<real, N> v_a;
    VaryingFieldSpan<real> v_rho;
    VaryingFieldSpan<real> v_m;
    VaryingFieldSpan<real, N> v_n;

    static bool selects(Group const &group) {
      return (group.GetGroupType() == "fluid");
    }
  };

private:
  struct groups_boundary_data {
    size_t _count;
    size_t index;

    // uniform fields
    UniformFieldSpan<real> u_beta;

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
  aat13() {
    this->RegisterProcedure(
        "compute_particle_normal", &aat13::compute_particle_normal);
    this->RegisterProcedure(
        "accumulate_acceleration", &aat13::accumulate_acceleration);
  }

public:
  std::string GetFullName() const override { return GetFullNameImpl(); }

public:
  void Load(Model &model) final {
    // global fields
    _data.global.g_h = model.AddGlobalFieldImpl<real>("smoothing_scale");

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
        data.u_gamma = group.AddUniformFieldImpl<real>("surface_tension");

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_a = group.AddVaryingFieldImpl<real, N>("acceleration");
        data.v_rho = group.AddVaryingFieldImpl<real>("density");
        data.v_m = group.AddVaryingFieldImpl<real>("mass");
        data.v_n = group.AddVaryingFieldImpl<real, N>("aat13_particle_normal");
      }

      if (groups_boundary_data::selects(group)) {
        auto &data = _data.groups.boundary.emplace_back();

        data._count = group.GetItemCount();
        data.index = group_index;

        // uniform fields
        data.u_beta = group.AddUniformFieldImpl<real>("adhesion");

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_V = group.AddVaryingFieldImpl<real>("volume");
      }
    }
  }

public:
  void compute_particle_normal(Neighborhood const &nhood) {
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
            p.v_n[i] = o::template zeros<real, N>();

            { // foreach fluid neighbor f_f
              for (auto &n : _data.groups.fluid) {
                for (auto const j : neighbors[n.index]) {
                  // compute
                  p.v_n[i] +=
                      (((o::kernel_support_radius<kernel_type>(*g.g_h) *
                         n.v_m[j]) /
                        n.v_rho[j]) *
                       o::kernel_gradient_h<kernel_type>(
                           (p.v_x[i] - n.v_x[j]), *g.g_h));
                }
              }
            } // foreach fluid neighbor f_f
          }
        }
      } // omp parallel region
    }   // foreach fluid particle f
  }

public:
  void accumulate_acceleration(Neighborhood const &nhood) {
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

            { // foreach fluid neighbor f_f
              for (auto &n : _data.groups.fluid) {
                for (auto const j : neighbors[n.index]) {
                  // compute
                  p.v_a[i] -=
                      (((((((static_cast<T>(2) * *p.u_rho0) /
                            (p.v_rho[i] + n.v_rho[j])) *
                           *p.u_gamma) *
                          n.v_m[j]) *
                         (p.v_x[i] - n.v_x[j])) *
                        o::reciprocal_or_zero(
                            o::norm((p.v_x[i] - n.v_x[j])),
                            static_cast<T>(1e-9))) *
                       o::aat13_cohesion_h(
                           o::norm((p.v_x[i] - n.v_x[j])), *g.g_h));

                  // compute
                  p.v_a[i] -=
                      ((((static_cast<T>(2) * *p.u_rho0) /
                         (p.v_rho[i] + n.v_rho[j])) *
                        *p.u_gamma) *
                       (p.v_n[i] - n.v_n[j]));
                }
              }
            } // foreach fluid neighbor f_f

            { // foreach boundary neighbor f_b
              for (auto &n : _data.groups.boundary) {
                for (auto const j : neighbors[n.index]) {
                  // compute
                  p.v_a[i] -=
                      (((((*n.u_beta * *p.u_rho0) * n.v_V[j]) *
                         (p.v_x[i] - n.v_x[j])) *
                        o::reciprocal_or_zero(
                            o::norm((p.v_x[i] - n.v_x[j])),
                            static_cast<T>(1e-9))) *
                       o::aat13_adhesion_h(
                           o::norm((p.v_x[i] - n.v_x[j])), *g.g_h));
                }
              }
            } // foreach boundary neighbor f_b
          }
        }
      } // omp parallel region
    }   // foreach fluid particle f
  }

private:
  static std::string GetFullNameImpl() {
    std::ostringstream ss;
    ss << "prtcl::schemes::aat13";
    ss << "[T=" << MakeComponentType<T>() << ", N=" << N
       << ", K=" << kernel_type::get_name() << "]";
    return ss.str();
  }

public:
  std::string_view GetPrtclSourceCode() const final {
    return R"prtcl(

scheme aat13 {
  groups fluid {
    select type fluid;

    varying field x = real[] position;
    varying field a = real[] acceleration;

    varying field rho = real density;
    varying field m = real mass;

    uniform field rho0 = real rest_density;

    uniform field gamma = real surface_tension;

    varying field n = real[] aat13_particle_normal;
  }

  groups boundary {
    select type boundary;

    varying field x = real[] position;

    varying field V = real volume;

    uniform field beta = real adhesion;
  }

  global {
    field h = real smoothing_scale;
  }

  procedure compute_particle_normal {
    foreach fluid particle f {
      compute n.f = zeros<real[]>();

      foreach fluid neighbor f_f {
        compute n.f +=
            kernel_support_radius(h)
          *
            m.f_f / rho.f_f
          *
            kernel_gradient_h(x.f - x.f_f, h);
      }
    }
  }

  procedure accumulate_acceleration {
    foreach fluid particle f {
      foreach fluid neighbor f_f {
        // cohesion
        compute a.f -=
            (
                (2 * rho0.f / (rho.f + rho.f_f))
              *
                gamma.f
              *
                m.f_f
              *
                (x.f - x.f_f)
              *
                reciprocal_or_zero(norm(x.f - x.f_f), 1e-9)
            )
          *
            aat13_cohesion_h(norm(x.f - x.f_f), h);

        // curvature
        compute a.f -=
            (
                (2 * rho0.f / (rho.f + rho.f_f))
              *
                gamma.f
            )
          *
            (n.f - n.f_f);
      }

      foreach boundary neighbor f_b {
        compute a.f -=
            (
                beta.f_b
              *
                rho0.f * V.f_b
              *
                (x.f - x.f_b)
              *
                reciprocal_or_zero(norm(x.f - x.f_b), 1e-9)
            )
          *
            aat13_adhesion_h(norm(x.f - x.f_b), h);
      }
    }
  }
}


)prtcl";
  }

private:
  static inline bool const registered_ =
      GetSchemeRegistry().RegisterScheme<aat13>(GetFullNameImpl());

  friend void Register_aat13();
};
} // namespace schemes
} // namespace prtcl
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
