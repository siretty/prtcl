#pragma once

#define PRTCL_ASSERT(...)
#define PRTCL_INLINE

#include "../common.hpp"

#include "../log/logger.hpp"
#include "../log/trace.hpp"

#include "CompactNSearch/CompactNSearch.hpp"

#include <boost/range/algorithm/copy.hpp>

namespace prtcl::rt {

template <typename ModelPolicy_> class compact_n_search_grid {
public:
  using model_policy = ModelPolicy_;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;

private:
  using real = typename type_policy::template dtype_t<nd_dtype::real>;

private:
  static constexpr size_t N = model_policy::dimensionality;

  static_assert(N == 3, "");

  using impl_type = CompactNSearch::NeighborhoodSearch;

public:
  compact_n_search_grid(real radius = 1) : _nsearch{radius, false} {}

public:
  real get_radius() const { return _nsearch.radius(); }

  void set_radius(real radius) { _nsearch.set_radius(radius); }

public:
  // update(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update(GroupedVectorData const &x) {
    PRTCL_RT_LOG_TRACE_SCOPED("compact_n_search update");

    size_t current_group_count = get_group_count(x);

    if (_nsearch.n_point_sets() > current_group_count)
      throw "internal error: removing groups is not supported";

    // add new point sets if neccessary
    if (_nsearch.n_point_sets() < current_group_count) {
      for (size_t g = _nsearch.n_point_sets(); g < current_group_count; ++g) {
        auto &group = get_group_ref(x, g);
        size_t group_size = get_element_count(group);

        log::debug(
            "neighbor", "grid", "adding group ", g, " with ", group_size,
            " elements");

        if (group_size == 0) {
          _nsearch.add_point_set(nullptr, 0, true, true, true, nullptr);
        } else {
          auto &first = get_element_ref(group, 0);
          _nsearch.add_point_set(
              &first[0], group_size, true, true, true, nullptr);
        }
      }
    }

    // resize point sets if neccessary
    for (size_t g = 0; g < get_group_count(x); ++g) {
      size_t point_set_size = _nsearch.point_set(g).n_points();

      auto &group = get_group_ref(x, g);
      size_t group_size = get_element_count(group);

      if (group_size != point_set_size) {
        log::debug(
            "neighbor", "grid", "resizing group ", g, " from ", point_set_size,
            " to ", group_size, " elements");

        auto &first = get_element_ref(group, 0);
        _nsearch.resize_point_set(g, &first[0], group_size);
      }
    }

    _nsearch.update_point_sets();
    _nsearch.update_activation_table();
  }

  // }}}

public:
  // compute_group_permutations(range_) {{{

  template <typename Range_> void compute_group_permutations(Range_ &range) {
    // reorder the particles accoring to a z_curve
    _nsearch.z_sort();
    // extract the permutations
    for (size_t g = 0; g < _nsearch.n_point_sets(); ++g)
      boost::range::copy(_nsearch.point_set(g).get_sort_table(), range[g]);
  }

  // }}}

public:
  // neighbors(group, index, data, callback) {{{

  template <typename GroupedVectorData, typename Fn>
  void neighbors(
      size_t group, size_t index, GroupedVectorData const &, Fn fn) const {
    _nsearch.find_neighbors(group, index, fn);
  }

  // }}}

  // neighbors(position, data, callback) {{{

  template <typename X_, typename GroupedVectorData_, typename Fn_>
  void neighbors(X_ &x, GroupedVectorData_ const &, Fn_ fn) const {
    _nsearch.find_neighbors(&x[0], fn);
  }

  // }}}

private:
  impl_type _nsearch;
};

} // namespace prtcl::rt

// NOTE:
//    This adapts a modified version of
//    https://github.com/InteractiveComputerGraphics/CompactNSearch
//    to the uniform-grid interface used by prtcl.
