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
class correction : public SchemeBase {
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

    // varying fields
    VaryingFieldSpan<real, N> v_x;
    VaryingFieldSpan<real> v_rho;
    VaryingFieldSpan<real> v_m;
    VaryingFieldSpan<real, N, N> v_L;

    static bool selects(Group const &group) {
      return (group.GetGroupType() == "fluid");
    }
  };

private:
  struct {
    global_data global;

    struct {
      std::vector<groups_fluid_data> fluid;
    } groups;

    size_t group_count;
  } _data;

private:
  struct per_thread_data {
    std::vector<std::vector<size_t>> neighbors;
  };

  std::vector<per_thread_data> _per_thread;

public:
  correction() {
    this->RegisterProcedure(
        "compute_gradient_correction",
        &correction::compute_gradient_correction);
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

    for (size_t group_index = 0; group_index < group_count; ++group_index) {
      auto &group = model.GetGroups()[group_index];

      if (groups_fluid_data::selects(group)) {
        auto &data = _data.groups.fluid.emplace_back();

        data._count = group.GetItemCount();
        data.index = group_index;

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_rho = group.AddVaryingFieldImpl<real>("density");
        data.v_m = group.AddVaryingFieldImpl<real>("mass");
        data.v_L = group.AddVaryingFieldImpl<real, N, N>("gradient_correction");
      }
    }
  }

public:
  void compute_gradient_correction(Neighborhood const &nhood) {
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

            // local_def ...;
            Tensor<real, N, N> l_L_f = o::template zeros<real, N, N>();

            { // foreach fluid neighbor f_f
              for (auto &n : _data.groups.fluid) {
                for (auto const j : neighbors[n.index]) {
                  // compute
                  l_L_f +=
                      ((n.v_m[j] / p.v_rho[i]) *
                       o::outer_product(
                           o::kernel_gradient_h<kernel_type>(
                               (p.v_x[i] - n.v_x[j]), *g.g_h),
                           (n.v_x[j] - p.v_x[i])));
                }
              }
            } // foreach fluid neighbor f_f

            // compute
            p.v_L[i] = o::invert_pm(l_L_f);
          }
        }
      } // omp parallel region
    }   // foreach fluid particle f
  }

private:
  static std::string GetFullNameImpl() {
    std::ostringstream ss;
    ss << "prtcl::schemes::correction";
    ss << "[T=" << MakeComponentType<T>() << ", N=" << N
       << ", K=" << kernel_type::get_name() << "]";
    return ss.str();
  }

private:
  static inline bool const registered_ =
      GetSchemeRegistry().RegisterScheme<correction>(GetFullNameImpl());

  friend void Register_correction();
};
} // namespace schemes
} // namespace prtcl
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
