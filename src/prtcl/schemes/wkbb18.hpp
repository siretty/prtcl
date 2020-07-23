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
class wkbb18 : public SchemeBase {
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
    UniformFieldSpan<real> u_mu;
    UniformFieldSpan<real> u_pcg_max_error;
    UniformFieldSpan<integer> u_pcg_max_iters;
    UniformFieldSpan<integer> u_pcg_iterations;

    // varying fields
    VaryingFieldSpan<real, N> v_x;
    VaryingFieldSpan<real, N> v_v;
    VaryingFieldSpan<real, N> v_a;
    VaryingFieldSpan<real> v_rho;
    VaryingFieldSpan<real> v_m;
    VaryingFieldSpan<real, N> v_vd;
    VaryingFieldSpan<real, N, N> v_sp;
    VaryingFieldSpan<real> v_t_birth;

    static bool selects(Group const &group) {
      return (group.GetGroupType() == "fluid");
    }
  };

private:
  struct groups_boundary_data {
    size_t _count;
    size_t index;

    // uniform fields
    UniformFieldSpan<real> u_mu;

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
  wkbb18() {
    this->RegisterProcedure("compute_diagonal", &wkbb18::compute_diagonal);
    this->RegisterProcedure(
        "accumulate_acceleration", &wkbb18::accumulate_acceleration);
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
        data.u_mu = group.AddUniformFieldImpl<real>("dynamic_viscosity");
        data.u_pcg_max_error =
            group.AddUniformFieldImpl<real>("wkbb18_maximum_error");
        data.u_pcg_max_iters =
            group.AddUniformFieldImpl<integer>("wkbb18_maximum_iterations");
        data.u_pcg_iterations =
            group.AddUniformFieldImpl<integer>("wkbb18_iterations");

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_v = group.AddVaryingFieldImpl<real, N>("velocity");
        data.v_a = group.AddVaryingFieldImpl<real, N>("acceleration");
        data.v_rho = group.AddVaryingFieldImpl<real>("density");
        data.v_m = group.AddVaryingFieldImpl<real>("mass");
        data.v_vd = group.AddVaryingFieldImpl<real, N>("wkbb18_velocity_delta");
        data.v_sp =
            group.AddVaryingFieldImpl<real, N, N>("wkbb18_system_diagonal");
        data.v_t_birth = group.AddVaryingFieldImpl<real>("time_of_birth");
      }

      if (groups_boundary_data::selects(group)) {
        auto &data = _data.groups.boundary.emplace_back();

        data._count = group.GetItemCount();
        data.index = group_index;

        // uniform fields
        data.u_mu = group.AddUniformFieldImpl<real>("dynamic_viscosity");

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_V = group.AddVaryingFieldImpl<real>("volume");
      }
    }
  }

public:
  void compute_diagonal(Neighborhood const &nhood) {
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
            p.v_sp[i] = o::template zeros<real, N, N>();

            { // foreach @particle@ neighbor f_f
              auto &n = p;

              for (auto const j : neighbors[n.index]) {
                // compute
                p.v_sp[i] +=
                    ((((static_cast<T>(10) * (*p.u_mu / p.v_rho[i])) *
                       (n.v_m[j] / n.v_rho[j])) /
                      (o::norm_squared((p.v_x[i] - n.v_x[j])) +
                       ((static_cast<T>(0.01) * *g.g_h) * *g.g_h))) *
                     o::outer_product(
                         o::kernel_gradient_h<kernel_type>(
                             (p.v_x[i] - n.v_x[j]), *g.g_h),
                         (p.v_x[i] - n.v_x[j])));
              }
            } // foreach @particle@ neighbor f_f

            { // foreach boundary neighbor f_b
              for (auto &n : _data.groups.boundary) {
                for (auto const j : neighbors[n.index]) {
                  // compute
                  p.v_sp[i] +=
                      ((((static_cast<T>(10) * (*n.u_mu / p.v_rho[i])) *
                         ((*p.u_rho0 * n.v_V[j]) / p.v_rho[i])) /
                        (o::norm_squared((p.v_x[i] - n.v_x[j])) +
                         ((static_cast<T>(0.01) * *g.g_h) * *g.g_h))) *
                       o::outer_product(
                           o::kernel_gradient_h<kernel_type>(
                               (p.v_x[i] - n.v_x[j]), *g.g_h),
                           (p.v_x[i] - n.v_x[j])));
                }
              }
            } // foreach boundary neighbor f_b

            // compute
            p.v_sp[i] = o::invert(
                (o::template identity<real, N, N>() - (*g.g_dt * p.v_sp[i])));
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

    { // solve_pcg ...
      // the solver object
      static CGOpenMP<T, N> solver;

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

          // fetch the neighbor storage for this thread
          auto &neighbors = _per_thread[omp_get_thread_num()].neighbors;

          // cleanup neighbor storage
          for (auto &pgn : neighbors)
            pgn.clear();

          // find all neighbors of (p, i)
          nhood.CopyNeighbors(p.index, i, neighbors);

          // local_def ...;
          Tensor<real, N> l_a_b = o::template zeros<real, N>();

          // local_def ...;
          Tensor<real, N> l_v_f_b = o::template zeros<real, N>();

          { // foreach boundary neighbor f_b
            for (auto &n : _data.groups.boundary) {
              for (auto const j : neighbors[n.index]) {
                // compute
                l_a_b +=
                    (((((static_cast<T>(10) * (*n.u_mu / p.v_rho[i])) *
                        ((*p.u_rho0 * n.v_V[j]) / p.v_rho[i])) *
                       o::dot(l_v_f_b, (p.v_x[i] - n.v_x[j]))) /
                      (o::norm_squared((p.v_x[i] - n.v_x[j])) +
                       ((static_cast<T>(0.01) * *g.g_h) * *g.g_h))) *
                     o::kernel_gradient_h<kernel_type>(
                         (p.v_x[i] - n.v_x[j]), *g.g_h));
              }
            }
          } // foreach boundary neighbor f_b

          // compute
          p_into[i] = (p.v_v[i] - (*g.g_dt * l_a_b));
        };

        auto setup_g = [&](auto &p, size_t i, auto &p_into) {
          // solve_setup

          // compute
          p_into[i] = (p.v_v[i] + p.v_vd[i]);
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

          // local_def ...;
          Tensor<real, N> l_a_f = o::template zeros<real, N>();

          { // foreach @particle@ neighbor f_f
            auto &n = p;

            for (auto const j : neighbors[n.index]) {
              // compute
              l_a_f +=
                  (((((static_cast<T>(10) * (*p.u_mu / p.v_rho[i])) *
                      (n.v_m[j] / n.v_rho[j])) *
                     o::dot((p_with[i] - p_with[j]), (p.v_x[i] - n.v_x[j]))) /
                    (o::norm_squared((p.v_x[i] - n.v_x[j])) +
                     ((static_cast<T>(0.01) * *g.g_h) * *g.g_h))) *
                   o::kernel_gradient_h<kernel_type>(
                       (p.v_x[i] - n.v_x[j]), *g.g_h));
            }
          } // foreach @particle@ neighbor f_f

          { // foreach boundary neighbor f_b
            for (auto &n : _data.groups.boundary) {
              for (auto const j : neighbors[n.index]) {
                // compute
                l_a_f +=
                    (((((static_cast<T>(10) * (*n.u_mu / p.v_rho[i])) *
                        ((*p.u_rho0 * n.v_V[j]) / p.v_rho[i])) *
                       o::dot(p_with[i], (p.v_x[i] - n.v_x[j]))) /
                      (o::norm_squared((p.v_x[i] - n.v_x[j])) +
                       ((static_cast<T>(0.01) * *g.g_h) * *g.g_h))) *
                     o::kernel_gradient_h<kernel_type>(
                         (p.v_x[i] - n.v_x[j]), *g.g_h));
              }
            }
          } // foreach boundary neighbor f_b

          // compute
          p_into[i] = (p_with[i] - (*g.g_dt * l_a_f));
        };

        auto product_p = [&](auto &p, size_t i, auto &p_into, auto &p_with) {
          // solve_product

          // compute
          p_into[i] = (p.v_sp[i] * p_with[i]);
        };

        auto apply = [&](auto &p, size_t i, auto &p_with) {
          // solve_apply

          // compute
          p.v_vd[i] =
              ((p_with[i] - p.v_v[i]) *
               o::unit_step_l(
                   static_cast<T>(0),
                   ((*g.g_t - p.v_t_birth[i]) - *g.g_dt_fade)));

          // compute
          p.v_a[i] = (p.v_vd[i] / *g.g_dt);
        };

        auto const iterations = solver.solve(
            p, setup_r, setup_g, product_s, product_p, apply,
            *p.u_pcg_max_error, *p.u_pcg_max_iters);

        *p.u_pcg_iterations = iterations;

        log::Debug("scheme", "scheme", "pcg solver iterations ", iterations);
      }

    } // ... solve_pcg
  }

private:
  static std::string GetFullNameImpl() {
    std::ostringstream ss;
    ss << "prtcl::schemes::wkbb18";
    ss << "[T=" << MakeComponentType<T>() << ", N=" << N
       << ", K=" << kernel_type::get_name() << "]";
    return ss.str();
  }

public:
  std::string_view GetPrtclSourceCode() const final {
    return R"prtcl(

scheme wkbb18 {
  groups fluid {
    select type fluid;

    varying field x = real[] position;
    varying field v = real[] velocity;
    varying field a = real[] acceleration;

    varying field rho = real density;
    varying field m   = real mass;

    uniform field rho0 = real rest_density;
    uniform field mu   = real dynamic_viscosity;

    varying field vd = real[]   wkbb18_velocity_delta;
    varying field sp = real[][] wkbb18_system_diagonal;

    varying field t_birth = real time_of_birth;

    // TODO: at the moment the aliases
    //       - pcg_max_error
    //       - pcg_max_iters
    //       - pcg_iterations
    //       are hardcoded (the field name can be freely chosen)
    uniform field pcg_max_error  = real    wkbb18_maximum_error;
    uniform field pcg_max_iters  = integer wkbb18_maximum_iterations;
    uniform field pcg_iterations = integer wkbb18_iterations;
  }

  groups boundary {
    select type boundary;

    varying field x = real[] position;

    varying field V = real volume;
    
    uniform field mu = real dynamic_viscosity;
  }

  global {
    field h = real smoothing_scale;
    field dt = real time_step;

    field t = real current_time;
    field dt_fade = real fade_duration;
  }

  procedure compute_diagonal {
    foreach fluid particle f {
      // reset the diagonal element vector
      compute sp.f = zeros<real[][]>();

      // accumulate over fluid neighbors
      foreach _ neighbor f_f {
        compute sp.f +=
            10 // HACK: this is actually a dimensionality correction factor (10 in 3D, 8 in 2D)
          *
            (mu.f / rho.f)
          *
            (m.f_f / rho.f_f)
          /
            (norm_squared(x.f - x.f_f) + 0.01 * h * h)
          *
            outer_product(
              kernel_gradient_h(x.f - x.f_f, h),
              x.f - x.f_f
            )
          ;
      }

      // accumulate over boundary neighbors
      foreach boundary neighbor f_b {
        compute sp.f +=
            10 // HACK: this is actually a dimensionality correction factor (10 in 3D, 8 in 2D)
          *
            (mu.f_b / rho.f)
          *
            (rho0.f * V.f_b / rho.f)
          /
            (norm_squared(x.f - x.f_b) + 0.01 * h * h)
          *
            outer_product(
              kernel_gradient_h(x.f - x.f_b, h),
              x.f - x.f_b
            )
        ;
      }

      compute sp.f = invert(identity<real[][]>() - dt * sp.f);
    }
  }

  procedure accumulate_acceleration {
    solve pcg real[] over fluid particle f {
      setup right_hand_side into result {
        local a_b : real[] = zeros<real[]>();
        local v_f_b : real[] = zeros<real[]>();

        foreach boundary neighbor f_b {
          compute a_b += 
              10 // HACK: this is actually a dimensionality correction factor (10 in 3D, 8 in 2D)
            *
              (mu.f_b / rho.f)
            *
              (rho0.f * V.f_b / rho.f)
            *
              dot(v_f_b, x.f - x.f_b)
            /
              (norm_squared(x.f - x.f_b) + 0.01 * h * h)
            *
              kernel_gradient_h(x.f - x.f_b, h);
        }

        compute result.f = v.f - dt * a_b;
      }

      setup guess into result {
        compute result.f = v.f + vd.f;
      }

      product preconditioner with iterate into result {
        compute result.f = sp.f * iterate.f;
      }

      product system with iterate into result {
        local a_f : real[] = zeros<real[]>();

        foreach _ neighbor f_f {
          compute a_f += 
              10 // HACK: this is actually a dimensionality correction factor (10 in 3D, 8 in 2D)
            *
              (mu.f / rho.f)
            *
              (m.f_f / rho.f_f)
            *
              dot(iterate.f - iterate.f_f, x.f - x.f_f)
            /
              (norm_squared(x.f - x.f_f) + 0.01 * h * h)
            *
              kernel_gradient_h(x.f - x.f_f, h);
        }

        foreach boundary neighbor f_b {
          compute a_f += 
              10 // HACK: this is actually a dimensionality correction factor (10 in 3D, 8 in 2D)
            *
              (mu.f_b / rho.f)
            *
              (rho0.f * V.f_b / rho.f)
            *
              dot(iterate.f /* - v.f_b */, x.f - x.f_b)
            /
              (norm_squared(x.f - x.f_b) + 0.01 * h * h)
            *
              kernel_gradient_h(x.f - x.f_b, h);
        }

        compute result.f = iterate.f - dt * a_f;
      }

      apply iterate {
        // velocity delta is set to zero if the particle is still fading in
        compute vd.f = (iterate.f - v.f)
          * unit_step_l(0, (t - t_birth.f) - dt_fade);
        // compute the viscosity acceleration
        compute a.f = vd.f / dt;
      }
    }
  }
}

)prtcl";
  }

private:
  static inline bool const registered_ =
      GetSchemeRegistry().RegisterScheme<wkbb18>(GetFullNameImpl());

  friend void Register_wkbb18();
};
} // namespace schemes
} // namespace prtcl
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
