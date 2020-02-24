
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
class test {
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
    nd_dtype_data_ref_t<nd_dtype::integer> global_neighbor_count;
    nd_dtype_data_ref_t<nd_dtype::integer> global_particle_count;

    static void _require(model_type &m_) {
      m_.template add_global<nd_dtype::integer>("global_neighbor_count");
      m_.template add_global<nd_dtype::integer>("global_particle_count");
    }

    void _load(model_type const &m_) {
      global_neighbor_count = m_.template get_global<nd_dtype::integer>("global_neighbor_count");
      global_particle_count = m_.template get_global<nd_dtype::integer>("global_particle_count");
    }
  };

private:
  struct particles_data {
    // particle count of the selected group
    size_t _count;
    // index of the selected group
    size_t _index;

    static void _require(group_type &g_) {
    }

    void _load(group_type const &g_) {
      _count = g_.size();
    }
  };

private:
  struct neighbors_data {
    // particle count of the selected group
    size_t _count;
    // index of the selected group
    size_t _index;

    static void _require(group_type &g_) {
    }

    void _load(group_type const &g_) {
      _count = g_.size();
    }
  };

public:
  static void require(model_type &m_) {
    global_data::_require(m_);
    
    for (auto &group : m_.groups()) {
      if (group.get_type() == "particles") {
        particles_data::_require(group);
      }
      if (group.get_type() == "neighbors") {
        neighbors_data::_require(group);
      }
    }
  }
  
public:
  void load(model_type &m_) {
    _group_count = m_.groups().size();

    _data.global._load(m_);

    _data.by_group_type.particles.clear();
    _data.by_group_type.neighbors.clear();

    auto groups = m_.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      auto &group = groups[static_cast<typename decltype(groups)::difference_type>(i)];

      if (group.get_type() == "particles") {
        auto &data = _data.by_group_type.particles.emplace_back();
        data._load(group);
        data._index = i;
      }

      if (group.get_type() == "neighbors") {
        auto &data = _data.by_group_type.neighbors.emplace_back();
        data._load(group);
        data._index = i;
      }
    }
  }
  
private:
  struct {
    global_data global;
    struct {
      std::vector<particles_data> particles;
      std::vector<neighbors_data> neighbors;
    } by_group_type;
  } _data;

  struct per_thread_type {
    std::vector<std::vector<size_t>> neighbors;

    // reductions
    nd_dtype_t<nd_dtype::integer> rd_global_neighbor_count;
    nd_dtype_t<nd_dtype::integer> rd_global_particle_count;
  };

  std::vector<per_thread_type> _per_thread;
  size_t _group_count;

public:
  template <typename NHood_>
  void test_counting_particles(NHood_ const &nhood_) {
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

      // select and initialize this threads reduction variable
      auto &rd_global_particle_count = _per_thread[thread_index].rd_global_particle_count;
      rd_global_particle_count = c::template zeros<nd_dtype::integer>();

      for (auto &p : _data.by_group_type.particles) {
#pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          // no neighbours neccessary

          rd_global_particle_count += l::template narray<nd_dtype::integer>({1});
        }
      }

      _Pragma("omp critical") {
        // combine all reduction variables

        g.global_particle_count[0] += rd_global_particle_count;
      } // pragma omp critical
    } // pragma omp parallel
  }

public:
  template <typename NHood_>
  void test_counting_neighbors(NHood_ const &nhood_) {
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

      // select and initialize this threads reduction variable
      auto &rd_global_neighbor_count = _per_thread[thread_index].rd_global_neighbor_count;
      rd_global_neighbor_count = c::template zeros<nd_dtype::integer>();

      for (auto &p : _data.by_group_type.particles) {
#pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          // clean up the neighbor storage
          for (auto &pgn : neighbors)
            pgn.clear();

          // find all neighbors of (p, i)
          nhood_.neighbors(p._index, i, [&neighbors](auto n_index, auto j) {
            neighbors[n_index].push_back(j);
          });

          for (auto &n : _data.by_group_type.neighbors) {
            for (auto const j : neighbors[n._index]) {
              rd_global_neighbor_count += l::template narray<nd_dtype::integer>({1});
            }
          }
        }
      }

      _Pragma("omp critical") {
        // combine all reduction variables

        g.global_neighbor_count[0] += rd_global_neighbor_count;
      } // pragma omp critical
    } // pragma omp parallel
  }
};

} /* namespace schemes*/ } /* namespace prtcl*/


#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
  