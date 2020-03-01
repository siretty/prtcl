
#pragma once

#include <prtcl/rt/basic_group.hpp>
#include <prtcl/rt/basic_model.hpp>
#include <prtcl/rt/common.hpp>

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
class aat13 {
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
    nd_dtype_data_ref_t<nd_dtype::real> smoothing_scale;

    static void _require(model_type &m_) {
      m_.template add_global<nd_dtype::real>("smoothing_scale");
    }

    void _load(model_type const &m_) {
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
    nd_dtype_data_ref_t<nd_dtype::real> surface_tension;
    nd_dtype_data_ref_t<nd_dtype::real> rest_density;

    // varying fields
    nd_dtype_data_ref_t<nd_dtype::real, N> acceleration;
    nd_dtype_data_ref_t<nd_dtype::real> mass;
    nd_dtype_data_ref_t<nd_dtype::real, N> aat13_particle_normal;
    nd_dtype_data_ref_t<nd_dtype::real> density;
    nd_dtype_data_ref_t<nd_dtype::real, N> position;

    static void _require(group_type &g_) {
      // uniform fields
      g_.template add_uniform<nd_dtype::real>("surface_tension");
      g_.template add_uniform<nd_dtype::real>("rest_density");

      // varying fields
      g_.template add_varying<nd_dtype::real, N>("acceleration");
      g_.template add_varying<nd_dtype::real>("mass");
      g_.template add_varying<nd_dtype::real, N>("aat13_particle_normal");
      g_.template add_varying<nd_dtype::real>("density");
      g_.template add_varying<nd_dtype::real, N>("position");
    }

    void _load(group_type const &g_) {
      _count = g_.size();

      // uniform fields
      surface_tension = g_.template get_uniform<nd_dtype::real>("surface_tension");
      rest_density = g_.template get_uniform<nd_dtype::real>("rest_density");

      // varying fields
      acceleration = g_.template get_varying<nd_dtype::real, N>("acceleration");
      mass = g_.template get_varying<nd_dtype::real>("mass");
      aat13_particle_normal = g_.template get_varying<nd_dtype::real, N>("aat13_particle_normal");
      density = g_.template get_varying<nd_dtype::real>("density");
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
    }
  }
  
public:
  void load(model_type &m_) {
    _group_count = m_.groups().size();

    _data.global._load(m_);

    _data.by_group_type.fluid.clear();

    auto groups = m_.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      auto &group = groups[static_cast<typename decltype(groups)::difference_type>(i)];

      if ((group.get_type() == "fluid") and (true)) {
        auto &data = _data.by_group_type.fluid.emplace_back();
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
  void compute_particle_normal(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member types
    using l = typename math_policy::literals;
    using c = typename math_policy::constants;
    using o = typename math_policy::operations;

    _Pragma("omp parallel") {
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

          p.aat13_particle_normal[i] = c::template zeros<nd_dtype::real, N>();

          for (auto &n : _data.by_group_type.fluid) {
            for (auto const j : neighbors[n._index]) {
              p.aat13_particle_normal[i] += (o::kernel_support_radius(g.smoothing_scale[0]) * n.mass[j] / n.density[j] * o::kernel_gradient_h((p.position[i] - n.position[j]), g.smoothing_scale[0]));
            }
          }
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

          for (auto &n : _data.by_group_type.fluid) {
            for (auto const j : neighbors[n._index]) {
              p.acceleration[i] -= ((l::template narray<nd_dtype::real>({2}) * p.rest_density[0] / (p.density[i] + n.density[j])) * p.surface_tension[0] * n.mass[j] * o::aat13_cohesion_h(o::norm((p.position[i] - n.position[j])), g.smoothing_scale[0]) * (p.position[i] - n.position[j]) / o::norm((p.position[i] - n.position[j])));

              p.acceleration[i] -= ((l::template narray<nd_dtype::real>({2}) * p.rest_density[0] / (p.density[i] + n.density[j])) * p.surface_tension[0] * (p.aat13_particle_normal[i] - n.aat13_particle_normal[j]));
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
  