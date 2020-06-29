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
class horas : public SchemeBase {
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
    UniformFieldSpan<real, N> g_x_min;
    UniformFieldSpan<real, N> g_x_max;
  };

private:
  struct groups_visible_data {
    size_t _count;
    size_t index;

    // uniform fields

    // varying fields
    VaryingFieldSpan<real, N> v_x;

    static bool selects(Group const &group) {
      return (group.HasTag("visible"));
    }
  };

private:
  struct groups_fluid_data {
    size_t _count;
    size_t index;

    // uniform fields
    UniformFieldSpan<real> u_rho0;

    // varying fields
    VaryingFieldSpan<real, N> v_x;
    VaryingFieldSpan<real> v_rho;
    VaryingFieldSpan<real> v_m;

    static bool selects(Group const &group) {
      return ((group.GetGroupType() == "fluid") and (group.HasTag("visible")));
    }
  };

private:
  struct groups_horason_data {
    size_t _count;
    size_t index;

    // uniform fields

    // varying fields
    VaryingFieldSpan<integer, 2> v_s;
    VaryingFieldSpan<real, N> v_x;
    VaryingFieldSpan<real, N> v_x0;
    VaryingFieldSpan<real, N> v_d;
    VaryingFieldSpan<real> v_t0;
    VaryingFieldSpan<real> v_t;
    VaryingFieldSpan<real> v_phi;
    VaryingFieldSpan<real, N> v_grad_phi;

    static bool selects(Group const &group) {
      return (group.GetGroupType() == "horason");
    }
  };

private:
  struct {
    global_data global;

    struct {
      std::vector<groups_visible_data> visible;
      std::vector<groups_fluid_data> fluid;
      std::vector<groups_horason_data> horason;
    } groups;

    size_t group_count;
  } _data;

private:
  struct per_thread_data {
    std::vector<std::vector<size_t>> neighbors;
  };

  std::vector<per_thread_data> _per_thread;

public:
  horas() {
    this->RegisterProcedure("update_visible_aabb", &horas::update_visible_aabb);
    this->RegisterProcedure("reset", &horas::reset);
    this->RegisterProcedure("step_fluid", &horas::step_fluid);
    this->RegisterProcedure("step", &horas::step);
  }

public:
  std::string GetFullName() const override { return GetFullNameImpl(); }

public:
  void Load(Model &model) final {
    // global fields
    _data.global.g_h = model.AddGlobalFieldImpl<real>("smoothing_scale");
    _data.global.g_x_min =
        model.AddGlobalFieldImpl<real, N>("position_aabb_min");
    _data.global.g_x_max =
        model.AddGlobalFieldImpl<real, N>("position_aabb_max");

    auto group_count = model.GetGroupCount();
    _data.group_count = group_count;

    _data.groups.visible.clear();
    _data.groups.fluid.clear();
    _data.groups.horason.clear();

    for (size_t group_index = 0; group_index < group_count; ++group_index) {
      auto &group = model.GetGroups()[group_index];

      if (groups_visible_data::selects(group)) {
        auto &data = _data.groups.visible.emplace_back();

        data._count = group.GetItemCount();
        data.index = group_index;

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
      }

      if (groups_fluid_data::selects(group)) {
        auto &data = _data.groups.fluid.emplace_back();

        data._count = group.GetItemCount();
        data.index = group_index;

        // uniform fields
        data.u_rho0 = group.AddUniformFieldImpl<real>("rest_density");

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_rho = group.AddVaryingFieldImpl<real>("density");
        data.v_m = group.AddVaryingFieldImpl<real>("mass");
      }

      if (groups_horason_data::selects(group)) {
        auto &data = _data.groups.horason.emplace_back();

        data._count = group.GetItemCount();
        data.index = group_index;

        // varying fields
        data.v_s = group.AddVaryingFieldImpl<integer, 2>("sensor_position");
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_x0 = group.AddVaryingFieldImpl<real, N>("initial_position");
        data.v_d = group.AddVaryingFieldImpl<real, N>("direction");
        data.v_t0 = group.AddVaryingFieldImpl<real>("initial_parameter");
        data.v_t = group.AddVaryingFieldImpl<real>("parameter");
        data.v_phi = group.AddVaryingFieldImpl<real>("implicit_function");
        data.v_grad_phi =
            group.AddVaryingFieldImpl<real, N>("implicit_function_gradient");
      }
    }
  }

public:
  void update_visible_aabb(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    // foreach dimension index dim
    for (size_t i_dim = 0; i_dim < N; ++i_dim) {
      // local_def ...;
      Tensor<real> l_x_min_dim = o::template positive_infinity<real>();

      // local_def ...;
      Tensor<real> l_x_max_dim = o::template negative_infinity<real>();

      { // foreach visible particle i

        // initialize reductions
        Tensor<real> r_l_x_min_dim = l_x_min_dim;
        Tensor<real> r_l_x_max_dim = l_x_max_dim;

#pragma omp parallel reduction(min                                             \
                               : r_l_x_min_dim) reduction(max                  \
                                                          : r_l_x_max_dim)
        {
          PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=visible");

          auto &t = _per_thread[omp_get_thread_num()];

          for (auto &p : _data.groups.visible) {
#pragma omp for schedule(static)
            for (size_t i = 0; i < p._count; ++i) {
              // reduce
              r_l_x_min_dim = o::min(r_l_x_min_dim, (p.v_x[i])[i_dim]);

              // reduce
              r_l_x_max_dim = o::max(r_l_x_max_dim, (p.v_x[i])[i_dim]);
            }
          }
        } // omp parallel region

        // finalize reductions
        l_x_min_dim = r_l_x_min_dim;
        l_x_max_dim = r_l_x_max_dim;

      } // foreach visible particle i

      // compute
      (*g.g_x_min)[i_dim] = (l_x_min_dim - *g.g_h);

      // compute
      (*g.g_x_max)[i_dim] = (l_x_max_dim + *g.g_h);
    } // foreach dimension index dim
  }

public:
  void reset(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    { // foreach horason particle i
#pragma omp parallel
      {
        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=horason");

        auto &t = _per_thread[omp_get_thread_num()];

        for (auto &p : _data.groups.horason) {
#pragma omp for schedule(static)
          for (size_t i = 0; i < p._count; ++i) {
            // compute
            p.v_x[i] = p.v_x0[i];

            // compute
            p.v_t[i] = p.v_t0[i];
          }
        }
      } // omp parallel region
    }   // foreach horason particle i
  }

public:
  void step_fluid(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    // local_def ...;
    Tensor<real> l_W = static_cast<T>(0.8);

    // local_def ...;
    Tensor<real> l_L = static_cast<T>(1.1);

    { // foreach horason particle i
#pragma omp parallel
      {
        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=horason");

        auto &t = _per_thread[omp_get_thread_num()];

        // select, resize and reserve neighbor storage
        auto &neighbors = t.neighbors;
        neighbors.resize(_data.group_count);
        for (auto &pgn : neighbors)
          pgn.reserve(100);

        for (auto &p : _data.groups.horason) {
#pragma omp for
          for (size_t i = 0; i < p._count; ++i) {
            // cleanup neighbor storage
            for (auto &pgn : neighbors)
              pgn.clear();

            // find all neighbors of (p, i)
            nhood.CopyNeighbors(p.index, i, neighbors);

            // compute
            p.v_phi[i] = o::template zeros<real>();

            // compute
            p.v_grad_phi[i] = o::template zeros<real, N>();

            { // foreach fluid neighbor f_i
              for (auto &n : _data.groups.fluid) {
                for (auto const j : neighbors[n.index]) {
                  // compute
                  p.v_phi[i] +=
                      ((n.v_m[j] / n.v_rho[j]) *
                       o::kernel_h<kernel_type>((p.v_x[i] - n.v_x[j]), *g.g_h));

                  // compute
                  p.v_grad_phi[i] +=
                      ((n.v_m[j] / n.v_rho[j]) *
                       o::kernel_gradient_h<kernel_type>(
                           (p.v_x[i] - n.v_x[j]), *g.g_h));
                }
              }
            } // foreach fluid neighbor f_i

            // compute
            p.v_phi[i] = ((l_W - p.v_phi[i]) * *g.g_h);

            // compute
            p.v_grad_phi[i] *= *g.g_h;

            // compute
            p.v_t[i] += (p.v_phi[i] / l_L);

            // compute
            p.v_x[i] = (p.v_x0[i] + (p.v_t[i] * p.v_d[i]));
          }
        }
      } // omp parallel region
    }   // foreach horason particle i
  }

public:
  void step(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    // local_def ...;
    Tensor<real> l_R = (static_cast<T>(2) * *g.g_h);

    // local_def ...;
    Tensor<real> l_W = (*g.g_h / static_cast<T>(2));

    // local_def ...;
    Tensor<real> l_O = *g.g_h;

    // local_def ...;
    Tensor<real> l_L = static_cast<T>(1.1);

    { // foreach horason particle i
#pragma omp parallel
      {
        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=horason");

        auto &t = _per_thread[omp_get_thread_num()];

        // select, resize and reserve neighbor storage
        auto &neighbors = t.neighbors;
        neighbors.resize(_data.group_count);
        for (auto &pgn : neighbors)
          pgn.reserve(100);

        for (auto &p : _data.groups.horason) {
#pragma omp for
          for (size_t i = 0; i < p._count; ++i) {
            // cleanup neighbor storage
            for (auto &pgn : neighbors)
              pgn.clear();

            // find all neighbors of (p, i)
            nhood.CopyNeighbors(p.index, i, neighbors);

            // local_def ...;
            Tensor<real, N> l_nom = o::template zeros<real, N>();

            // local_def ...;
            Tensor<real> l_den = static_cast<T>(0);

            { // foreach visible neighbor j
              for (auto &n : _data.groups.visible) {
                for (auto const j : neighbors[n.index]) {
                  // local_def ...;
                  Tensor<real> l_k = (o::norm((p.v_x[i] - n.v_x[j])) / l_R);

                  // compute
                  l_k = (static_cast<T>(1) - (l_k * l_k));

                  // compute
                  l_k = ((l_k * l_k) * l_k);

                  // compute
                  l_nom += (n.v_x[j] * l_k);

                  // compute
                  l_den += l_k;
                }
              }
            } // foreach visible neighbor j

            // local_def ...;
            Tensor<real, N> l_v_bar =
                (l_nom * o::reciprocal_or_zero(l_den, static_cast<T>(1e-9)));

            // compute
            p.v_phi[i] = o::min(
                (((o::norm((p.v_x[i] - l_v_bar)) - l_W) *
                  o::unit_step_r(static_cast<T>(1e-9), l_den)) +
                 (l_O * o::unit_step_r(l_den, static_cast<T>(1e-9)))),
                l_O);

            // compute
            p.v_t[i] += (p.v_phi[i] / l_L);

            // compute
            p.v_x[i] = (p.v_x0[i] + (p.v_t[i] * p.v_d[i]));
          }
        }
      } // omp parallel region
    }   // foreach horason particle i
  }

private:
  static std::string GetFullNameImpl() {
    std::ostringstream ss;
    ss << "prtcl::schemes::horas";
    ss << "[T=" << MakeComponentType<T>() << ", N=" << N
       << ", K=" << kernel_type::get_name() << "]";
    return ss.str();
  }

private:
  static inline bool const registered_ =
      GetSchemeRegistry().RegisterScheme<horas>(GetFullNameImpl());

  friend void Register_horas();
};
} // namespace schemes
} // namespace prtcl
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
