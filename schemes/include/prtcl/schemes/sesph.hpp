
#pragma once

#include <prtcl/rt/common.hpp>

#include <prtcl/rt/basic_group.hpp>
#include <prtcl/rt/basic_model.hpp>

#include <prtcl/rt/log/trace.hpp>

#include <vector>

#include <omp.h>

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

namespace prtcl { namespace schemes {

template <
  typename ModelPolicy_
>
class sesph {
public:
  using model_policy = ModelPolicy_;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;
  using data_policy = typename model_policy::data_policy;

  using nd_dtype = prtcl::rt::nd_dtype;

  template <nd_dtype DType_> using dtype_t = typename type_policy::template dtype_t<DType_>;
  template <nd_dtype DType_, size_t ...Ns_> using nd_dtype_t = typename math_policy::template nd_dtype_t<DType_, Ns_...>;
  template <nd_dtype DType_, size_t ...Ns_> using nd_dtype_data_ref_t = typename data_policy::template nd_dtype_data_ref_t<DType_, Ns_...>;

  static constexpr size_t N = model_policy::dimensionality;

  using model_type = prtcl::rt::basic_model<model_policy>;
  using group_type = prtcl::rt::basic_group<model_policy>;

private:
  struct global_data {
    nd_dtype_data_ref_t<nd_dtype::real, N> gravity;
    nd_dtype_data_ref_t<nd_dtype::real> smoothing_scale;

    static void _require(model_type &m_) {
      m_.template add_global<nd_dtype::real, N>("gravity");
      m_.template add_global<nd_dtype::real>("smoothing_scale");
    }

    void _load(model_type const &m_) {
      gravity = m_.template get_global<nd_dtype::real, N>("gravity");
      smoothing_scale = m_.template get_global<nd_dtype::real>("smoothing_scale");
    }
  };

private:
  struct fluid_data {
    // particle count of the selected group
    size_t _count;
    // index of the selected group
    size_t _index;

    // uniform fields
    nd_dtype_data_ref_t<nd_dtype::real> compressibility;
    nd_dtype_data_ref_t<nd_dtype::real> viscosity;
    nd_dtype_data_ref_t<nd_dtype::real> rest_density;

    // varying fields
    nd_dtype_data_ref_t<nd_dtype::real, N> acceleration;
    nd_dtype_data_ref_t<nd_dtype::real> mass;
    nd_dtype_data_ref_t<nd_dtype::real> pressure;
    nd_dtype_data_ref_t<nd_dtype::real> density;
    nd_dtype_data_ref_t<nd_dtype::real, N> velocity;
    nd_dtype_data_ref_t<nd_dtype::real, N> position;

    static void _require(group_type &g_) {
      // uniform fields
      g_.template add_uniform<nd_dtype::real>("compressibility");
      g_.template add_uniform<nd_dtype::real>("viscosity");
      g_.template add_uniform<nd_dtype::real>("rest_density");

      // varying fields
      g_.template add_varying<nd_dtype::real, N>("acceleration");
      g_.template add_varying<nd_dtype::real>("mass");
      g_.template add_varying<nd_dtype::real>("pressure");
      g_.template add_varying<nd_dtype::real>("density");
      g_.template add_varying<nd_dtype::real, N>("velocity");
      g_.template add_varying<nd_dtype::real, N>("position");
    }

    void _load(group_type const &g_) {
      _count = g_.size();

      // uniform fields
      compressibility = g_.template get_uniform<nd_dtype::real>("compressibility");
      viscosity = g_.template get_uniform<nd_dtype::real>("viscosity");
      rest_density = g_.template get_uniform<nd_dtype::real>("rest_density");

      // varying fields
      acceleration = g_.template get_varying<nd_dtype::real, N>("acceleration");
      mass = g_.template get_varying<nd_dtype::real>("mass");
      pressure = g_.template get_varying<nd_dtype::real>("pressure");
      density = g_.template get_varying<nd_dtype::real>("density");
      velocity = g_.template get_varying<nd_dtype::real, N>("velocity");
      position = g_.template get_varying<nd_dtype::real, N>("position");
    }
  };

private:
  struct boundary_data {
    // particle count of the selected group
    size_t _count;
    // index of the selected group
    size_t _index;

    // uniform fields
    nd_dtype_data_ref_t<nd_dtype::real> viscosity;

    // varying fields
    nd_dtype_data_ref_t<nd_dtype::real> volume;
    nd_dtype_data_ref_t<nd_dtype::real, N> velocity;
    nd_dtype_data_ref_t<nd_dtype::real, N> position;

    static void _require(group_type &g_) {
      // uniform fields
      g_.template add_uniform<nd_dtype::real>("viscosity");

      // varying fields
      g_.template add_varying<nd_dtype::real>("volume");
      g_.template add_varying<nd_dtype::real, N>("velocity");
      g_.template add_varying<nd_dtype::real, N>("position");
    }

    void _load(group_type const &g_) {
      _count = g_.size();

      // uniform fields
      viscosity = g_.template get_uniform<nd_dtype::real>("viscosity");

      // varying fields
      volume = g_.template get_varying<nd_dtype::real>("volume");
      velocity = g_.template get_varying<nd_dtype::real, N>("velocity");
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
      if ((group.get_type() == "boundary") and (true)) {
        boundary_data::_require(group);
      }
    }
  }
  
public:
  void load(model_type &m_) {
    _group_count = m_.groups().size();

    _data.global._load(m_);

    _data.by_group_type.fluid.clear();
    _data.by_group_type.boundary.clear();

    auto groups = m_.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      auto &group = groups[static_cast<typename decltype(groups)::difference_type>(i)];

      if ((group.get_type() == "fluid") and (true)) {
        auto &data = _data.by_group_type.fluid.emplace_back();
        data._load(group);
        data._index = i;
      }

      if ((group.get_type() == "boundary") and (true)) {
        auto &data = _data.by_group_type.boundary.emplace_back();
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
      std::vector<boundary_data> boundary;
    } by_group_type;
  } _data;

  struct per_thread_type {
    std::vector<std::vector<size_t>> neighbors;

    // reductions
  };

  std::vector<per_thread_type> _per_thread;
  size_t _group_count;

public:
  template <typename NHood_>
  void compute_density_and_pressure(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member types
    using l = typename math_policy::literals;
    using c = typename math_policy::constants;
    using o = typename math_policy::operations;

    _Pragma("omp parallel") {
      PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

      _Pragma("omp single") {
        auto const thread_count = static_cast<size_t>(omp_get_num_threads());
        _per_thread.resize(thread_count);
      } // pragma omp single

      auto const thread_index = static_cast<size_t>(omp_get_thread_num());

      // select and resize the neighbor storage for the current thread
      auto &neighbors = _per_thread[thread_index].neighbors;
      neighbors.resize(_group_count);

      for (auto &pgn : neighbors)
        pgn.reserve(100);

      for (auto &p : _data.by_group_type.fluid) {
#pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          // clean up the neighbor storage
          for (auto &pgn : neighbors)
            pgn.clear();

          // find all neighbors of (p, i)
          nhood_.neighbors(p._index, i, [&neighbors](auto n_index, auto j) {
            neighbors[n_index].push_back(j);
          });

          p.density[i] = l::template narray<nd_dtype::real>({0});

          {// foreach fluid neighbor f_f
            PRTCL_RT_LOG_TRACE_SCOPED("foreach_neighbor", "n=fluid");

            for (auto &n : _data.by_group_type.fluid) {
              PRTCL_RT_LOG_TRACE_PLOT_NUMBER("neighbor count", static_cast<int64_t>(neighbors[n._index].size()));

              for (auto const j : neighbors[n._index]) {
                p.density[i] += (n.mass[j] * o::kernel_h((p.position[i] - n.position[j]), g.smoothing_scale[0]));
              }
            }
          }

          {// foreach boundary neighbor f_b
            PRTCL_RT_LOG_TRACE_SCOPED("foreach_neighbor", "n=boundary");

            for (auto &n : _data.by_group_type.boundary) {
              PRTCL_RT_LOG_TRACE_PLOT_NUMBER("neighbor count", static_cast<int64_t>(neighbors[n._index].size()));

              for (auto const j : neighbors[n._index]) {
                p.density[i] += (n.volume[j] * p.rest_density[0] * o::kernel_h((p.position[i] - n.position[j]), g.smoothing_scale[0]));
              }
            }
          }

          p.pressure[i] = (p.compressibility[0] * o::max(l::template narray<nd_dtype::real>({0}), ((p.density[i] / p.rest_density[0]) - l::template narray<nd_dtype::real>({1}))));
        }
      }
    } // pragma omp parallel
  }

public:
  template <typename NHood_>
  void compute_acceleration(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member types
    using l = typename math_policy::literals;
    using c = typename math_policy::constants;
    using o = typename math_policy::operations;

    _Pragma("omp parallel") {
      PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=fluid");

      _Pragma("omp single") {
        auto const thread_count = static_cast<size_t>(omp_get_num_threads());
        _per_thread.resize(thread_count);
      } // pragma omp single

      auto const thread_index = static_cast<size_t>(omp_get_thread_num());

      // select and resize the neighbor storage for the current thread
      auto &neighbors = _per_thread[thread_index].neighbors;
      neighbors.resize(_group_count);

      for (auto &pgn : neighbors)
        pgn.reserve(100);

      for (auto &p : _data.by_group_type.fluid) {
#pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          // clean up the neighbor storage
          for (auto &pgn : neighbors)
            pgn.clear();

          // find all neighbors of (p, i)
          nhood_.neighbors(p._index, i, [&neighbors](auto n_index, auto j) {
            neighbors[n_index].push_back(j);
          });

          p.acceleration[i] = g.gravity[0];

          {// foreach fluid neighbor f_f
            PRTCL_RT_LOG_TRACE_SCOPED("foreach_neighbor", "n=fluid");

            for (auto &n : _data.by_group_type.fluid) {
              PRTCL_RT_LOG_TRACE_PLOT_NUMBER("neighbor count", static_cast<int64_t>(neighbors[n._index].size()));

              for (auto const j : neighbors[n._index]) {
                p.acceleration[i] += ((p.viscosity[0] * (n.mass[j] / n.density[j]) * o::dot((p.velocity[i] - n.velocity[j]), (p.position[i] - n.position[j])) / (o::norm_squared((p.position[i] - n.position[j])) + (l::template narray<nd_dtype::real>({0.01}) * g.smoothing_scale[0] * g.smoothing_scale[0]))) * o::kernel_gradient_h((p.position[i] - n.position[j]), g.smoothing_scale[0]));

                p.acceleration[i] -= (n.mass[j] * ((p.pressure[i] / (p.density[i] * p.density[i])) + (n.pressure[j] / (n.density[j] * n.density[j]))) * o::kernel_gradient_h((p.position[i] - n.position[j]), g.smoothing_scale[0]));
              }
            }
          }

          {// foreach boundary neighbor f_b
            PRTCL_RT_LOG_TRACE_SCOPED("foreach_neighbor", "n=boundary");

            for (auto &n : _data.by_group_type.boundary) {
              PRTCL_RT_LOG_TRACE_PLOT_NUMBER("neighbor count", static_cast<int64_t>(neighbors[n._index].size()));

              for (auto const j : neighbors[n._index]) {
                p.acceleration[i] += ((n.viscosity[0] * n.volume[j] * o::dot((p.velocity[i] - n.velocity[j]), (p.position[i] - n.position[j])) / (o::norm_squared((p.position[i] - n.position[j])) + (l::template narray<nd_dtype::real>({0.01}) * g.smoothing_scale[0] * g.smoothing_scale[0]))) * o::kernel_gradient_h((p.position[i] - n.position[j]), g.smoothing_scale[0]));

                p.acceleration[i] -= (l::template narray<nd_dtype::real>({0.7}) * n.volume[j] * p.rest_density[0] * (l::template narray<nd_dtype::real>({2}) * p.pressure[i] / (p.density[i] * p.density[i])) * o::kernel_gradient_h((p.position[i] - n.position[j]), g.smoothing_scale[0]));
              }
            }
          }
        }
      }
    } // pragma omp parallel
  }
};

} /* namespace schemes*/ } /* namespace prtcl*/


#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
  