#pragma once

#include <prtcl/rt/common.hpp>
#include <prtcl/rt/basic_model.hpp>
#include <prtcl/rt/basic_group.hpp>

#include <vector>

#include <omp.h>

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedef"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

namespace prtcl::schemes {

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
  struct neighbors_group_data {
    size_t _count;
    size_t _index;

    // uniform fields

    // varying fields

    static void _require(group_type &g_) {
      // uniform fields

      // varying fields
    }

    void _load(group_type const &g_) {
      _count = g_.size();

      // uniform fields

      // varying fields
    }
  };

private:
  struct particles_group_data {
    size_t _count;
    size_t _index;

    // uniform fields

    // varying fields

    static void _require(group_type &g_) {
      // uniform fields

      // varying fields
    }

    void _load(group_type const &g_) {
      _count = g_.size();

      // uniform fields

      // varying fields
    }
  };

public:
  static void require(model_type &m_) {
    global_data::_require(m_);

    for (auto &group : m_.groups()) {
      if (group.get_type() == "neighbors")
        neighbors_group_data::_require(group);
      if (group.get_type() == "particles")
        particles_group_data::_require(group);
    }
  }

public:
  void load(model_type &m_) {
    _group_count = m_.groups().size();

    _data.global._load(m_);

    auto groups = m_.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      auto &group = groups[static_cast<typename decltype(groups)::difference_type>(i)];

      if (group.get_type() == "neighbors") {
        auto &data = _data.by_group_type.neighbors.emplace_back();
        data._load(group);
        data._index = i;
      }

      if (group.get_type() == "particles") {
        auto &data = _data.by_group_type.particles.emplace_back();
        data._load(group);
        data._index = i;
      }
    }
  }

private:
  struct {
    global_data global;
    struct {
      std::vector<neighbors_group_data> neighbors;
      std::vector<particles_group_data> particles;
    } by_group_type;
  } _data;

  struct {
    std::vector<std::vector<std::vector<size_t>>> neighbors;

    // reductions
    std::vector<nd_dtype_t<nd_dtype::integer>> rd_global_neighbor_count;
    std::vector<nd_dtype_t<nd_dtype::integer>> rd_global_particle_count;
  } _per_thread;

  size_t _group_count;

  // start of child #0 procedure
public:
  template <typename NHood_>
  void test_counting_particles(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member (types)
    using o = typename math_policy::operations;
    using c = typename math_policy::constants;

    // start of child #0 foreach_particle
    #pragma omp parallel
    {
      #pragma omp single
      {
        auto const thread_count = static_cast<size_t>(omp_get_num_threads());

        _per_thread.rd_global_particle_count.resize(thread_count);
      } // pragma omp single

      auto const thread_index = static_cast<size_t>(omp_get_thread_num());

      // select the per-thread reduction variables
      auto &rd_global_particle_count = _per_thread.rd_global_particle_count[thread_index];

      // initialize the per-thread reduction variables
      rd_global_particle_count = c::template zeros<nd_dtype::integer>();

      // start of child #0 if_group_type
      for (auto &p : _data.by_group_type.particles) {
        #pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          // start of child #0 reduction
          rd_global_particle_count = ( rd_global_particle_count ) + ( static_cast<dtype_t<nd_dtype::integer>>(1) ) ;
          // close of child #0 reduction
        }
      }
      // close of child #0 if_group_type

      // combine global reductions
      #pragma omp critical
      {
        g.global_particle_count[0] = ( g.global_particle_count[0] ) + ( rd_global_particle_count ) ;
      } // pragma omp critical
    } // pragma omp parallel
    // close of child #0 foreach_particle
  }
  // close of child #0 procedure

  // start of child #1 procedure
public:
  template <typename NHood_>
  void test_counting_neighbors(NHood_ const &nhood_) {
    // alias for the global data
    auto &g = _data.global;

    // alias for the math_policy member (types)
    using o = typename math_policy::operations;
    using c = typename math_policy::constants;

    // start of child #0 foreach_particle
    #pragma omp parallel
    {
      #pragma omp single
      {
        auto const thread_count = static_cast<size_t>(omp_get_num_threads());

        _per_thread.neighbors.resize(thread_count);

        _per_thread.rd_global_neighbor_count.resize(thread_count);
      } // pragma omp single

      auto const thread_index = static_cast<size_t>(omp_get_thread_num());

      // select and resize the neighbor storage for the current thread
      auto &neighbors = _per_thread.neighbors[thread_index];
      neighbors.resize(_group_count);

      for (auto &pgn : neighbors)
        pgn.reserve(100);

      // select the per-thread reduction variables
      auto &rd_global_neighbor_count = _per_thread.rd_global_neighbor_count[thread_index];

      // initialize the per-thread reduction variables
      rd_global_neighbor_count = c::template zeros<nd_dtype::integer>();

      // start of child #0 if_group_type
      for (auto &p : _data.by_group_type.particles) {
        #pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          for (auto &pgn : neighbors)
            pgn.clear();

          bool has_neighbors = false;

          // start of child #0 foreach_neighbor
          if (!has_neighbors) {
            nhood_.neighbors(p._index, i, [&neighbors](auto n_index, auto j) {
              neighbors[n_index].push_back(j);
            });
            has_neighbors = true;
          }

          // start of child #0 if_group_type
          for (auto &n : _data.by_group_type.neighbors) {
            for (auto const j : neighbors[n._index]) {
              // start of child #0 reduction
              rd_global_neighbor_count = ( rd_global_neighbor_count ) + ( static_cast<dtype_t<nd_dtype::integer>>(1) ) ;
              // close of child #0 reduction
            }
          }
          // close of child #0 if_group_type
          // close of child #0 foreach_neighbor
        }
      }
      // close of child #0 if_group_type

      // combine global reductions
      #pragma omp critical
      {
        g.global_neighbor_count[0] = ( g.global_neighbor_count[0] ) + ( rd_global_neighbor_count ) ;
      } // pragma omp critical
    } // pragma omp parallel
    // close of child #0 foreach_particle
  }
  // close of child #1 procedure
};

} // namespace prtcl::schemes

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
