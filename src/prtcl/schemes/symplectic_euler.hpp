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
class symplectic_euler : public SchemeBase {
public:
  using real = T;
  using integer = int32_t;
  using boolean = bool;

  using kernel_type = K<T, N>;

  template <typename U, size_t... M>
  using Tensor = math::Tensor<U, M...>;

private:
  struct global_data {
    UniformFieldSpan<real> g_dt;
    UniformFieldSpan<real> g_t;
    UniformFieldSpan<real> g_dt_fade;
    UniformFieldSpan<real> g_max_speed;
  };

private:
  struct groups_dynamic_data {
    size_t _count;
    size_t index;

    // uniform fields

    // varying fields
    VaryingFieldSpan<real, N> v_x;
    VaryingFieldSpan<real, N> v_v;
    VaryingFieldSpan<real, N> v_a;
    VaryingFieldSpan<real> v_t_birth;

    static bool selects(Group const &group) {
      return (group.HasTag("dynamic"));
    }
  };

private:
  struct {
    global_data global;

    struct {
      std::vector<groups_dynamic_data> dynamic;
    } groups;

    size_t group_count;
  } _data;

private:
  struct per_thread_data {
    std::vector<std::vector<size_t>> neighbors;
  };

  std::vector<per_thread_data> _per_thread;

public:
  symplectic_euler() {
    this->RegisterProcedure(
        "integrate_velocity", &symplectic_euler::integrate_velocity);
    this->RegisterProcedure(
        "integrate_velocity_with_hard_fade",
        &symplectic_euler::integrate_velocity_with_hard_fade);
    this->RegisterProcedure(
        "integrate_velocity_with_smooth_fade",
        &symplectic_euler::integrate_velocity_with_smooth_fade);
    this->RegisterProcedure(
        "integrate_position", &symplectic_euler::integrate_position);
  }

public:
  std::string GetFullName() const override { return GetFullNameImpl(); }

public:
  void Load(Model &model) final {
    // global fields
    _data.global.g_dt = model.AddGlobalFieldImpl<real>("time_step");
    _data.global.g_t = model.AddGlobalFieldImpl<real>("current_time");
    _data.global.g_dt_fade = model.AddGlobalFieldImpl<real>("fade_duration");
    _data.global.g_max_speed = model.AddGlobalFieldImpl<real>("maximum_speed");

    auto group_count = model.GetGroupCount();
    _data.group_count = group_count;

    _data.groups.dynamic.clear();

    for (size_t group_index = 0; group_index < group_count; ++group_index) {
      auto &group = model.GetGroups()[group_index];

      if (groups_dynamic_data::selects(group)) {
        auto &data = _data.groups.dynamic.emplace_back();

        data._count = group.GetItemCount();
        data.index = group_index;

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_v = group.AddVaryingFieldImpl<real, N>("velocity");
        data.v_a = group.AddVaryingFieldImpl<real, N>("acceleration");
        data.v_t_birth = group.AddVaryingFieldImpl<real>("time_of_birth");
      }
    }
  }

public:
  void integrate_velocity(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    { // foreach dynamic particle i
#pragma omp parallel
      {
        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=dynamic");

        auto &t = _per_thread[omp_get_thread_num()];

        for (auto &p : _data.groups.dynamic) {
#pragma omp for schedule(static)
          for (size_t i = 0; i < p._count; ++i) {
            // compute
            p.v_v[i] += (*g.g_dt * p.v_a[i]);
          }
        }
      } // omp parallel region
    }   // foreach dynamic particle i
  }

public:
  void integrate_velocity_with_hard_fade(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    { // foreach dynamic particle i
#pragma omp parallel
      {
        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=dynamic");

        auto &t = _per_thread[omp_get_thread_num()];

        for (auto &p : _data.groups.dynamic) {
#pragma omp for schedule(static)
          for (size_t i = 0; i < p._count; ++i) {
            // compute
            p.v_v[i] +=
                ((*g.g_dt * p.v_a[i]) *
                 o::unit_step_l(
                     static_cast<T>(0),
                     ((*g.g_t - p.v_t_birth[i]) - *g.g_dt_fade)));
          }
        }
      } // omp parallel region
    }   // foreach dynamic particle i
  }

public:
  void integrate_velocity_with_smooth_fade(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    { // foreach dynamic particle i
#pragma omp parallel
      {
        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=dynamic");

        auto &t = _per_thread[omp_get_thread_num()];

        for (auto &p : _data.groups.dynamic) {
#pragma omp for schedule(static)
          for (size_t i = 0; i < p._count; ++i) {
            // compute
            p.v_v[i] +=
                ((*g.g_dt * p.v_a[i]) *
                 o::smoothstep(
                     (((*g.g_t - p.v_t_birth[i]) - *g.g_dt_fade) /
                      *g.g_dt_fade)));
          }
        }
      } // omp parallel region
    }   // foreach dynamic particle i
  }

public:
  void integrate_position(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    { // foreach dynamic particle i

      // initialize reductions
      Tensor<real> r_g_max_speed = *g.g_max_speed;

#pragma omp parallel reduction(max : r_g_max_speed)
      {
        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=dynamic");

        auto &t = _per_thread[omp_get_thread_num()];

        for (auto &p : _data.groups.dynamic) {
#pragma omp for schedule(static)
          for (size_t i = 0; i < p._count; ++i) {
            // compute
            p.v_x[i] += (*g.g_dt * p.v_v[i]);

            // reduce
            r_g_max_speed = o::max(r_g_max_speed, o::norm(p.v_v[i]));
          }
        }
      } // omp parallel region

      // finalize reductions
      *g.g_max_speed = r_g_max_speed;

    } // foreach dynamic particle i
  }

private:
  static std::string GetFullNameImpl() {
    std::ostringstream ss;
    ss << "prtcl::schemes::symplectic_euler";
    ss << "[T=" << MakeComponentType<T>() << ", N=" << N
       << ", K=" << kernel_type::get_name() << "]";
    return ss.str();
  }

private:
  static inline bool const registered_ =
      GetSchemeRegistry().RegisterScheme<symplectic_euler>(GetFullNameImpl());

  friend void Register_symplectic_euler();
};
} // namespace schemes
} // namespace prtcl
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
