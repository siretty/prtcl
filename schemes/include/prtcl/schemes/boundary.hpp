#pragma once

#include <prtcl/rt/common.hpp>
#include <prtcl/rt/basic_model.hpp>
#include <prtcl/rt/basic_group.hpp>

#include <vector>

#include <omp.h>

namespace prtcl::schemes {

template <
  typename ModelPolicy_
>
class boundary {
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
  struct boundary_group_data {
    size_t _count;
    size_t _index;

    // uniform fields

    // varying fields
    nd_dtype_data_ref_t<nd_dtype::real, N> position;
    nd_dtype_data_ref_t<nd_dtype::real> volume;

    static void _require(group_type &g_) {
      // uniform fields

      // varying fields
      g_.template add_varying<nd_dtype::real, N>("position");
      g_.template add_varying<nd_dtype::real>("volume");
    }

    void _load(group_type const &g_) {
      _count = g_.size();

      // uniform fields

      // varying fields
      position = g_.template get_varying<nd_dtype::real, N>("position");
      volume = g_.template get_varying<nd_dtype::real>("volume");
    }
  };

public:
  static void require(model_type &m_) {
    global_data::_require(m_);

    for (auto &group : m_.groups()) {
      if (group.get_type() == "boundary")
        boundary_group_data::_require(group);
    }
  }

public:
  void load(model_type &m_) {
    _group_count = m_.groups().size();

    _data.global._load(m_);

    auto groups = m_.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      auto &group = groups[static_cast<typename decltype(groups)::difference_type>(i)];

      if (group.get_type() == "boundary") {
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
      std::vector<boundary_group_data> boundary;
    } by_group_type;
  } _data;

  struct {
    std::vector<std::vector<std::vector<size_t>>> neighbors;

    // reductions
  } _per_thread;

  size_t _group_count;

  // start of child #0 procedure
public:
  template <typename NHood_>
  void compute_volume(NHood_ const &nhood_) {
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
      } // pragma omp single

      auto const thread_index = static_cast<size_t>(omp_get_thread_num());

      // select and resize the neighbor storage for the current thread
      auto &neighbors = _per_thread.neighbors[thread_index];
      neighbors.resize(_group_count);

      for (auto &pgn : neighbors)
        pgn.reserve(100);

      // start of child #0 if_group_type
      for (auto &p : _data.by_group_type.boundary) {
        #pragma omp for
        for (size_t i = 0; i < p._count; ++i) {
          for (auto &pgn : neighbors)
            pgn.clear();

          bool has_neighbors = false;

          // start of child #0 equation
          p.volume[i] = static_cast<dtype_t<nd_dtype::real>>(0.000000) ;
          // close of child #0 equation

          // start of child #1 foreach_neighbor
          if (!has_neighbors) {
            nhood_.neighbors(p._index, i, [&neighbors](auto n_index, auto j) {
              neighbors[n_index].push_back(j);
            });
            has_neighbors = true;
          }

          // start of child #0 if_group_type
          for (auto &n : _data.by_group_type.boundary) {
            for (auto const j : neighbors[n._index]) {
              // start of child #0 equation
              p.volume[i] += o::kernel_h( ( p.position[i] ) - ( n.position[j] ), g.smoothing_scale[0] ) ;
              // close of child #0 equation
            }
          }
          // close of child #0 if_group_type
          // close of child #1 foreach_neighbor

          // start of child #2 equation
          p.volume[i] = ( static_cast<dtype_t<nd_dtype::integer>>(1) ) / ( p.volume[i] ) ;
          // close of child #2 equation
        }
      }
      // close of child #0 if_group_type
    } // pragma omp parallel
    // close of child #0 foreach_particle
  }
  // close of child #0 procedure
};

} // namespace prtcl::schemes