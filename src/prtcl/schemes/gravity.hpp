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
class gravity : public SchemeBase {
public:
  using real = T;
  using integer = int32_t;
  using boolean = bool;

  using kernel_type = K<T, N>;

  template <typename U, size_t... M>
  using Tensor = math::Tensor<U, M...>;

private:
  struct global_data {
    UniformFieldSpan<real, N> g_g;
  };

private:
  struct groups_dynamic_data {
    size_t _count;
    size_t index;

    // uniform fields

    // varying fields
    VaryingFieldSpan<real, N> v_a;

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
  gravity() {
    this->RegisterProcedure(
        "initialize_acceleration", &gravity::initialize_acceleration);
    this->RegisterProcedure(
        "accumulate_acceleration", &gravity::accumulate_acceleration);
  }

public:
  std::string GetFullName() const override { return GetFullNameImpl(); }

public:
  void Load(Model &model) final {
    // global fields
    _data.global.g_g = model.AddGlobalFieldImpl<real, N>("gravity");

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
        data.v_a = group.AddVaryingFieldImpl<real, N>("acceleration");
      }
    }
  }

public:
  void initialize_acceleration(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    { // foreach dynamic particle f
#pragma omp parallel
      {
        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=dynamic");

        auto &t = _per_thread[omp_get_thread_num()];

        for (auto &p : _data.groups.dynamic) {
#pragma omp for schedule(static)
          for (size_t i = 0; i < p._count; ++i) {
            // compute
            p.v_a[i] = *g.g_g;
          }
        }
      } // omp parallel region
    }   // foreach dynamic particle f
  }

public:
  void accumulate_acceleration(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    { // foreach dynamic particle f
#pragma omp parallel
      {
        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=dynamic");

        auto &t = _per_thread[omp_get_thread_num()];

        for (auto &p : _data.groups.dynamic) {
#pragma omp for schedule(static)
          for (size_t i = 0; i < p._count; ++i) {
            // compute
            p.v_a[i] += *g.g_g;
          }
        }
      } // omp parallel region
    }   // foreach dynamic particle f
  }

private:
  static std::string GetFullNameImpl() {
    std::ostringstream ss;
    ss << "prtcl::schemes::gravity";
    ss << "[T=" << MakeComponentType<T>() << ", N=" << N
       << ", K=" << kernel_type::get_name() << "]";
    return ss.str();
  }

private:
  static inline bool const registered_ =
      GetSchemeRegistry().RegisterScheme<gravity>(GetFullNameImpl());

  friend void Register_gravity();
};
} // namespace schemes
} // namespace prtcl
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif