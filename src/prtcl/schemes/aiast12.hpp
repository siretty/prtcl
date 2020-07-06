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
class aiast12 : public SchemeBase {
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
  aiast12() {
    this->RegisterProcedure("compute_volume", &aiast12::compute_volume);
  }

public:
  std::string GetFullName() const override { return GetFullNameImpl(); }

public:
  void Load(Model &model) final {
    // global fields
    _data.global.g_h = model.AddGlobalFieldImpl<real>("smoothing_scale");

    auto group_count = model.GetGroupCount();
    _data.group_count = group_count;

    _data.groups.boundary.clear();

    for (size_t group_index = 0; group_index < group_count; ++group_index) {
      auto &group = model.GetGroups()[group_index];

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
  void compute_volume(Neighborhood const &nhood) {
    auto &g = _data.global;

    // mathematical operations
    namespace o = ::prtcl::math;

    // resize per-thread storage
    _per_thread.resize(omp_get_max_threads());

    { // foreach boundary particle b
#pragma omp parallel
      {
        PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=boundary");

        auto &t = _per_thread[omp_get_thread_num()];

        // select, resize and reserve neighbor storage
        auto &neighbors = t.neighbors;
        neighbors.resize(_data.group_count);
        for (auto &pgn : neighbors)
          pgn.reserve(100);

        for (auto &p : _data.groups.boundary) {
#pragma omp for
          for (size_t i = 0; i < p._count; ++i) {
            // cleanup neighbor storage
            for (auto &pgn : neighbors)
              pgn.clear();

            // find all neighbors of (p, i)
            nhood.CopyNeighbors(p.index, i, neighbors);

            // compute
            p.v_V[i] = static_cast<T>(0);

            { // foreach boundary neighbor b_b
              for (auto &n : _data.groups.boundary) {
                for (auto const j : neighbors[n.index]) {
                  // compute
                  p.v_V[i] +=
                      o::kernel_h<kernel_type>((p.v_x[i] - n.v_x[j]), *g.g_h);
                }
              }
            } // foreach boundary neighbor b_b

            // compute
            p.v_V[i] = (static_cast<T>(1) / p.v_V[i]);
          }
        }
      } // omp parallel region
    }   // foreach boundary particle b
  }

private:
  static std::string GetFullNameImpl() {
    std::ostringstream ss;
    ss << "prtcl::schemes::aiast12";
    ss << "[T=" << MakeComponentType<T>() << ", N=" << N
       << ", K=" << kernel_type::get_name() << "]";
    return ss.str();
  }

public:
  std::string_view GetPrtclSourceCode() const final {
    return R"prtcl(

scheme aiast12 {
  groups boundary {
    select type boundary;

    varying field x = real[] position;
    varying field V = real volume;
  }

  global {
    field h = real smoothing_scale;
  }

  procedure compute_volume {
    foreach boundary particle b {
      compute V.b = 0;

      foreach boundary neighbor b_b {
        compute V.b += kernel_h(x.b - x.b_b, h);
      }

      compute V.b = 1 / V.b;
    }
  }
}


)prtcl";
  }

private:
  static inline bool const registered_ =
      GetSchemeRegistry().RegisterScheme<aiast12>(GetFullNameImpl());

  friend void Register_aiast12();
};
} // namespace schemes
} // namespace prtcl
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
