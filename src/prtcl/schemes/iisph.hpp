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
class iisph : public SchemeBase {
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
    UniformFieldSpan<real> g_h;
    UniformFieldSpan<real> g_dt;
    UniformFieldSpan<real> g_omega;
    UniformFieldSpan<real> g_aprde;
    UniformFieldSpan<integer> g_nprde;
  };

private:
  struct groups_fluid_data {
    size_t _count;
    size_t index;

    // uniform fields
    UniformFieldSpan<real> u_rho0;

    // varying fields
    VaryingFieldSpan<real, N> v_x;
    VaryingFieldSpan<real, N> v_v;
    VaryingFieldSpan<real, N> v_a;
    VaryingFieldSpan<real> v_rho;
    VaryingFieldSpan<real> v_p;
    VaryingFieldSpan<real> v_m;
    VaryingFieldSpan<real, N> v_c;
    VaryingFieldSpan<real> v_s;
    VaryingFieldSpan<real> v_AA;
    VaryingFieldSpan<real> v_Ap;

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
  iisph() {
    this->RegisterProcedure("setup", &iisph::setup);
    this->RegisterProcedure(
        "iteration_pressure_acceleration",
        &iisph::iteration_pressure_acceleration);
    this->RegisterProcedure("iteration_pressure", &iisph::iteration_pressure);
  }

public:
  std::string GetFullName() const override { return GetFullNameImpl(); }

public:
  void Load(Model &model) final {
    // global fields
    _data.global.g_g = model.AddGlobalFieldImpl<real, N>("gravity");
    _data.global.g_h = model.AddGlobalFieldImpl<real>("smoothing_scale");
    _data.global.g_dt = model.AddGlobalFieldImpl<real>("time_step");
    _data.global.g_omega = model.AddGlobalFieldImpl<real>("iisph_relaxation");
    _data.global.g_aprde = model.AddGlobalFieldImpl<real>("iisph_aprde");
    _data.global.g_nprde = model.AddGlobalFieldImpl<integer>("iisph_nprde");

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

        // varying fields
        data.v_x = group.AddVaryingFieldImpl<real, N>("position");
        data.v_v = group.AddVaryingFieldImpl<real, N>("velocity");
        data.v_a = group.AddVaryingFieldImpl<real, N>("acceleration");
        data.v_rho = group.AddVaryingFieldImpl<real>("density");
        data.v_p = group.AddVaryingFieldImpl<real>("pressure");
        data.v_m = group.AddVaryingFieldImpl<real>("mass");
        data.v_c = group.AddVaryingFieldImpl<real, N>("iisph_helper_c");
        data.v_s = group.AddVaryingFieldImpl<real>("iisph_source_term");
        data.v_AA = group.AddVaryingFieldImpl<real>("iisph_diagonal_element");
        data.v_Ap = group.AddVaryingFieldImpl<real>("iisph_right_hand_side");
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
        p.v_c[i] = o::template zeros<real, N>();

        { // foreach fluid neighbor f_f
          for (auto &n : _data.groups.fluid) {
            for (auto const j : neighbors[n.index]) {
              // compute
              p.v_c[i] -=
                  ((n.v_m[j] / (p.v_rho[i] * p.v_rho[i])) *
                   o::kernel_gradient_h<kernel_type>(
                       (p.v_x[i] - n.v_x[j]), *g.g_h));
            }
          }
        } // foreach fluid neighbor f_f

        { // foreach boundary neighbor f_b
          for (auto &n : _data.groups.boundary) {
            for (auto const j : neighbors[n.index]) {
              // compute
              p.v_c[i] -=
                  ((((static_cast<T>(0.7) * static_cast<T>(2)) * *p.u_rho0) *
                    (n.v_V[j] / (p.v_rho[i] * p.v_rho[i]))) *
                   o::kernel_gradient_h<kernel_type>(
                       (p.v_x[i] - n.v_x[j]), *g.g_h));
            }
          }
        } // foreach boundary neighbor f_b
      }
    }
  } // omp parallel region
} // foreach fluid particle f

    // compute
    *g.g_nprde = o::template zeros<integer>();

{ // foreach fluid particle f

  // initialize reductions
  Tensor<integer> r_g_nprde = *g.g_nprde;

#pragma omp parallel reduction(+ : r_g_nprde)
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
        p.v_p[i] = static_cast<T>(0);

        // compute
        p.v_s[i] = static_cast<T>(0);

        // compute
        p.v_AA[i] = static_cast<T>(0);

        { // foreach fluid neighbor f_f
          for (auto &n : _data.groups.fluid) {
            for (auto const j : neighbors[n.index]) {
              // compute
              p.v_s[i] +=
                  (n.v_m[j] * o::dot(
                                  (p.v_v[i] - n.v_v[j]),
                                  o::kernel_gradient_h<kernel_type>(
                                      (p.v_x[i] - n.v_x[j]), *g.g_h)));

              // compute
              p.v_AA[i] +=
                  (n.v_m[j] *
                   o::dot(
                       p.v_c[i], o::kernel_gradient_h<kernel_type>(
                                     (p.v_x[i] - n.v_x[j]), *g.g_h)));

              // compute
              p.v_AA[i] -=
                  ((n.v_m[j] * (p.v_m[i] / (p.v_rho[i] * p.v_rho[i]))) *
                   o::norm_squared(o::kernel_gradient_h<kernel_type>(
                       (p.v_x[i] - n.v_x[j]), *g.g_h)));
            }
          }
        } // foreach fluid neighbor f_f

        { // foreach boundary neighbor f_b
          for (auto &n : _data.groups.boundary) {
            for (auto const j : neighbors[n.index]) {
              // compute
              p.v_s[i] +=
                  ((*p.u_rho0 * n.v_V[j]) *
                   o::dot(
                       (p.v_v[i] - o::template zeros<real, N>()),
                       o::kernel_gradient_h<kernel_type>(
                           (p.v_x[i] - n.v_x[j]), *g.g_h)));

              // compute
              p.v_AA[i] +=
                  ((*p.u_rho0 * n.v_V[j]) *
                   o::dot(
                       p.v_c[i], o::kernel_gradient_h<kernel_type>(
                                     (p.v_x[i] - n.v_x[j]), *g.g_h)));
            }
          }
        } // foreach boundary neighbor f_b

        // compute
        p.v_s[i] =
            ((*p.u_rho0 - p.v_rho[i]) -
             ((*g.g_dt * (*p.u_rho0 / p.v_rho[i])) * p.v_s[i]));

        // compute
        p.v_AA[i] *= ((*g.g_dt * *g.g_dt) * (*p.u_rho0 / p.v_rho[i]));

        // reduce
        r_g_nprde += o::template ones<integer>();
      }
    }
  } // omp parallel region

  // finalize reductions
  *g.g_nprde = r_g_nprde;

} // foreach fluid particle f
} // namespace schemes

public:
void iteration_pressure_acceleration(Neighborhood const &nhood) {
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
          p.v_a[i] = o::template zeros<real, N>();

          { // foreach fluid neighbor f_f
            for (auto &n : _data.groups.fluid) {
              for (auto const j : neighbors[n.index]) {
                // compute
                p.v_a[i] -=
                    ((n.v_m[j] * ((p.v_p[i] / (p.v_rho[i] * p.v_rho[i])) +
                                  (n.v_p[j] / (n.v_rho[j] * n.v_rho[j])))) *
                     o::kernel_gradient_h<kernel_type>(
                         (p.v_x[i] - n.v_x[j]), *g.g_h));
              }
            }
          } // foreach fluid neighbor f_f

          { // foreach boundary neighbor f_b
            for (auto &n : _data.groups.boundary) {
              for (auto const j : neighbors[n.index]) {
                // compute
                p.v_a[i] -=
                    ((((static_cast<T>(0.7) * *p.u_rho0) * n.v_V[j]) *
                      ((static_cast<T>(2) * p.v_p[i]) /
                       (p.v_rho[i] * p.v_rho[i]))) *
                     o::kernel_gradient_h<kernel_type>(
                         (p.v_x[i] - n.v_x[j]), *g.g_h));
              }
            }
          } // foreach boundary neighbor f_b
        }
      }
    } // omp parallel region
  }   // foreach fluid particle f
}

public:
void iteration_pressure(Neighborhood const &nhood) {
  auto &g = _data.global;

  // mathematical operations
  namespace o = ::prtcl::math;

  // resize per-thread storage
  _per_thread.resize(omp_get_max_threads());

  { // foreach fluid particle f

    // initialize reductions
    Tensor<real> r_g_aprde = *g.g_aprde;

#pragma omp parallel reduction(+ : r_g_aprde)
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
          p.v_Ap[i] = static_cast<T>(0);

          { // foreach fluid neighbor f_f
            for (auto &n : _data.groups.fluid) {
              for (auto const j : neighbors[n.index]) {
                // compute
                p.v_Ap[i] +=
                    (n.v_m[j] * o::dot(
                                    (p.v_a[i] - n.v_a[j]),
                                    o::kernel_gradient_h<kernel_type>(
                                        (p.v_x[i] - n.v_x[j]), *g.g_h)));
              }
            }
          } // foreach fluid neighbor f_f

          { // foreach boundary neighbor f_b
            for (auto &n : _data.groups.boundary) {
              for (auto const j : neighbors[n.index]) {
                // compute
                p.v_Ap[i] +=
                    ((*p.u_rho0 * n.v_V[j]) *
                     o::dot(
                         (p.v_a[i] - o::template zeros<real, N>()),
                         o::kernel_gradient_h<kernel_type>(
                             (p.v_x[i] - n.v_x[j]), *g.g_h)));
              }
            }
          } // foreach boundary neighbor f_b

          // compute
          p.v_Ap[i] *= ((*g.g_dt * *g.g_dt) * (*p.u_rho0 / p.v_rho[i]));

          // compute
          p.v_p[i] =
              (o::max(
                   static_cast<T>(0),
                   (p.v_p[i] +
                    ((*g.g_omega * (p.v_s[i] - p.v_Ap[i])) *
                     o::reciprocal_or_zero(p.v_AA[i], static_cast<T>(1e-9))))) *
               o::unit_step_l(static_cast<T>(1e-9), o::cabs(p.v_AA[i])));

          // reduce
          r_g_aprde +=
              (((p.v_Ap[i] - p.v_s[i]) / *p.u_rho0) *
               o::unit_step_l(static_cast<T>(1e-9), p.v_p[i]));
        }
      }
    } // omp parallel region

    // finalize reductions
    *g.g_aprde = r_g_aprde;

  } // foreach fluid particle f
}

private:
static std::string GetFullNameImpl() {
  std::ostringstream ss;
  ss << "prtcl::schemes::iisph";
  ss << "[T=" << MakeComponentType<T>() << ", N=" << N
     << ", K=" << kernel_type::get_name() << "]";
  return ss.str();
}

public:
std::string_view GetPrtclSourceCode() const final {
  return R"prtcl(

scheme iisph {
  groups fluid {
    select type fluid;

    varying field x = real[] position;
    varying field v = real[] velocity;
    varying field a = real[] acceleration;

    varying field rho = real density;
    varying field p   = real pressure;
    varying field m   = real mass;

    uniform field rho0 = real rest_density;

    // IISPH
    varying field c  = real[] iisph_helper_c;
    varying field s  = real   iisph_source_term;
    varying field AA = real   iisph_diagonal_element;
    varying field Ap = real   iisph_right_hand_side;
  }

  groups boundary {
    select type boundary;

    varying field x = real[] position;

    varying field V = real volume;
  }

  global {
    field g = real[] gravity;

    field h = real smoothing_scale;
    field dt = real time_step;

    // IISPH
    field omega = real iisph_relaxation;
    field aprde = real iisph_aprde;
    field nprde = integer iisph_nprde;
  }

  procedure setup {
    foreach fluid particle f {
      // reset the helper variable
      compute c.f = zeros<real[]>();

      foreach fluid neighbor f_f {
        // accumulate the helper variable
        compute c.f -=
            ( m.f_f / (rho.f * rho.f) )
          *
            kernel_gradient_h(x.f - x.f_f, h);
      }

      foreach boundary neighbor f_b {
        // accumulate the helper variable
        compute c.f -=
            // TODO: check the correction factor
            0.7 * 2
          *
            rho0.f * ( V.f_b / (rho.f * rho.f) )
          *
            kernel_gradient_h(x.f - x.f_b, h);
      }
    }

    compute nprde = zeros<integer>();

    foreach fluid particle f {
      // reset the pressure
      compute p.f = 0;

      // reset the source term
      compute s.f = 0;
      // reset the diagonal element
      compute AA.f = 0;

      foreach fluid neighbor f_f {
        // accumulate source term
        compute s.f +=
            m.f_f
          *
            dot(
              v.f - v.f_f,
              kernel_gradient_h(x.f - x.f_f, h)
            );
        // accumulate first term of the diagonal element
        compute AA.f +=
            m.f_f
          *
            dot(
              c.f,
              kernel_gradient_h(x.f - x.f_f, h)
            );
        // accumulate second term of the diagonal element
        compute AA.f -=
            m.f_f
          *
            ( m.f / (rho.f * rho.f) )
          *
            norm_squared(
              kernel_gradient_h(x.f - x.f_f, h)
            );
      }

      foreach boundary neighbor f_b {
        // accumulate source term
        compute s.f +=
            rho0.f * V.f_b
          *
            dot(
              // HACK: boundaries are currently static, adjust
              v.f - zeros<real[]>(),
              kernel_gradient_h(x.f - x.f_b, h)
            );
        // accumulate third term of the diagonal element
        compute AA.f +=
            rho0.f * V.f_b
          *
            dot(
              c.f,
              kernel_gradient_h(x.f - x.f_b, h)
            );
      }

      // finalize source term
      compute s.f = rho0.f - rho.f - dt * (rho0.f / rho.f) * s.f;
      // finalize diagonal element
      compute AA.f *= dt * dt * (rho0.f / rho.f);

      // accumulate the number of particles
      reduce nprde += ones<integer>();
    }
  }

  procedure iteration_pressure_acceleration {
    foreach fluid particle f {
      // reset the acceleration
      compute a.f = zeros<real[]>();

      foreach fluid neighbor f_f {
        // accumulate the pressure acceleration
        compute a.f -= 
            m.f_f
          *
            (p.f / (rho.f * rho.f) + p.f_f / (rho.f_f * rho.f_f))
          *
            kernel_gradient_h(x.f - x.f_f, h);                                             
      }

      foreach boundary neighbor f_b {
        // accumulate the pressure acceleration
        compute a.f -= 
            0.7
          *
            rho0.f * V.f_b 
          *
            (2 * p.f / (rho.f * rho.f))
          *
            kernel_gradient_h(x.f - x.f_b, h);                                             
      }
    }
  }

  procedure iteration_pressure {
    foreach fluid particle f {
      // reset the right hand side
      compute Ap.f = 0;

      foreach fluid neighbor f_f {
        // accumulate the right hand side
        compute Ap.f += 
            m.f_f
          *
            dot(
              a.f - a.f_f,
              kernel_gradient_h(x.f - x.f_f, h)
            );                                             
      }

      foreach boundary neighbor f_b {
        // accumulate the right hand side
        compute Ap.f += 
            rho0.f * V.f_b 
          *
            dot(
              a.f - zeros<real[]>(), // TODO: boundary currently has no acceleration
              kernel_gradient_h(x.f - x.f_b, h)
            );                                             
      }

      // finalize the right hand side
      compute Ap.f *= dt * dt * ( rho0.f / rho.f );

      // compute the pressure
      compute p.f =
          max(
            0,
              p.f
            +
                omega
              *
                (s.f - Ap.f)
              *
                reciprocal_or_zero(AA.f, 1e-9)
          )
        *
          unit_step_l(1e-9, cabs(AA.f));

      // accumulate the relative density error
      reduce aprde +=
          ( (Ap.f - s.f) / rho0.f )
        *
          unit_step_l(1e-9, p.f);
    }
  }
}


)prtcl";
}

private:
static inline bool const registered_ =
    GetSchemeRegistry().RegisterScheme<iisph>(GetFullNameImpl());

friend void Register_iisph();
}; // namespace prtcl
}
}
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
