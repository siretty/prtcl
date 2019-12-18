#pragma once

#include "../../data/integral_grid.hpp"
#include "../../math/morton_order.hpp"
#include "../../math/traits/host_math_traits.hpp"
#include "compact_uniform_grid/spatial_hash.hpp"
#include "compact_uniform_grid/spinlock.hpp"

#include <array>
#include <functional>
#include <unordered_map>

#include <cstddef>

#include <boost/container_hash/hash.hpp>

#include <iostream>

#include <Tracy.hpp>

namespace prtcl {

//#define COMPACT_UNIFORM_GRID_PARALLEL

template <typename T, size_t N> class compact_uniform_grid {
  using math_traits = host_math_traits<T, N>;

public:
  using scalar_type = typename math_traits::scalar_type;
  static constexpr size_t vector_extent = N;

private:
  using index_type = int32_t;
  using index_vector_type = std::array<index_type, N>;

  struct entry {
    std::vector<index_type> indices;
  };

  struct per_group {
    std::vector<entry> cell_to_entry;
    std::vector<index_vector_type> keys, old_keys;
  };

public:
  compact_uniform_grid(scalar_type radius_ = 1) : _radius{radius_} {}

public:
  scalar_type get_radius() const { return _radius; }

  void set_radius(scalar_type const radius_) { _radius = radius_; }

public:
  // initialize(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void initialize(GroupedVectorData const &x) {
    _cell_count = 0;

    _grid_to_cell.clear();

    _per_group.clear();
    _per_group.resize(get_group_count(x));

    for (size_t gi = 0; gi < get_group_count(x); ++gi) {
      auto const &gd = get_group_ref(x, gi);

      // resize the key storage
      _per_group[gi].keys.resize(get_element_count(gd));
      _per_group[gi].old_keys.resize(get_element_count(gd));

      for (size_t ri = 0; ri < get_element_count(gd); ++ri) {
        index_vector_type key = compute_grid_index(x, gi, ri);

        // store the computed key
        _per_group[gi].keys[ri] = key;
        _per_group[gi].old_keys[ri] = key;

        // check if the grid_index already has a cell
        auto it = _grid_to_cell.find(key);

        // grid_index has no cell yet
        if (it == _grid_to_cell.end()) {
          // find the next available index for cells
          auto cell_index = _cell_count++;

          // resize the per-group cell_to_entry maps
          for (auto &pg : _per_group) {
            pg.cell_to_entry.resize(cell_index + 1);
            pg.cell_to_entry[cell_index].indices.reserve(50);
          }

          // insert the cell index into the map
          it = _grid_to_cell.insert({key, cell_index}).first;
        }

        // add the raw index to the cell
        _per_group[gi]
            .cell_to_entry[static_cast<size_t>(it->second)]
            .indices.push_back(static_cast<index_type>(ri));
      }
    }

    _initialized = true;
  }

  // }}}

  // update(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update(GroupedVectorData const &x) {
    if (!_initialized) {
      this->initialize(x);
      return;
    }

    // flip key storage
    for (auto &pg : _per_group)
      std::swap(pg.keys, pg.old_keys);

    {
      ZoneScopedN("update keys");
#pragma omp parallel
      {
        for (size_t gi = 0; gi < get_group_count(x); ++gi) {
          auto const &gd = get_group_ref(x, gi);

#pragma omp for
          for (size_t ri = 0; ri < get_element_count(gd); ++ri) {
            // recompute key
            _per_group[gi].keys[ri] = compute_grid_index(x, gi, ri);
          }
        }
      }
    }

    {
      ZoneScopedN("update maps");

      for (size_t gi = 0; gi < get_group_count(x); ++gi) {
        auto const &gd = get_group_ref(x, gi);

        for (size_t ri = 0; ri < get_element_count(gd); ++ri) {
          index_vector_type key = _per_group[gi].keys[ri];
          index_vector_type old_key = _per_group[gi].old_keys[ri];

          // nothing to do, if the grid key did not change
          if (key == old_key)
            continue;

          // find the cell of this key
          auto it = _grid_to_cell.find(key);

          // grid_index has no cell yet
          if (it == _grid_to_cell.end()) {
            // find the next available index for cells
            auto cell_index = _cell_count++;

            // resize the per-group cell_to_entry maps
            for (auto &pg : _per_group) {
              pg.cell_to_entry.resize(cell_index + 1);
            }

            // insert the cell index into the map
            it = _grid_to_cell.insert({key, cell_index}).first;
          }

          // std::cout << "pg[" << gi << "].cell_to_entry[" << it->second <<
          // "]\n";

          { // add the raw index to the cell
            auto &entry =
                _per_group[gi].cell_to_entry[static_cast<size_t>(it->second)];
            entry.indices.push_back(static_cast<index_type>(ri));
          }

          // remove the raw index from the old cell
          if (auto old_it = _grid_to_cell.find(old_key);
              old_it != _grid_to_cell.end()) {
            auto &entry =
                _per_group[gi]
                    .cell_to_entry[static_cast<size_t>(old_it->second)];
            auto &indices = entry.indices;
            if (auto index_it = std::find(indices.begin(), indices.end(), ri);
                index_it != indices.end()) {
              indices.erase(index_it);
            }
          } else
            throw "internal error: old key has no cell";
        }
      }
    }
  }

  // }}}

public:
  template <typename OutputIt>
  void compute_group_permutation(size_t group, OutputIt it) const {
    (void)(group), (void)(it);
    throw "not implemented";
  }

public:
  template <typename GroupedVectorData, typename Fn>
  void neighbours(size_t group, size_t index, GroupedVectorData const &x,
                  Fn &&fn) const {
    auto p1 = get_element_ref(get_group_ref(x, group), index);
    auto radius_sq = _radius * _radius;
    this->potential_neighbours(
        group, index, [&p1, &fn, &x, radius_sq](size_t gi, size_t ri) {
          auto p2 = get_element_ref(get_group_ref(x, gi), ri);
          if (math_traits::norm_squared(p1 - p2) < radius_sq)
            std::invoke(std::forward<Fn>(fn), gi, ri);
        });
  }

  template <typename Fn>
  void potential_neighbours(size_t group, size_t index, Fn &&fn) const {
    this->potential_neighbours(_per_group[group].keys[index],
                               std::forward<Fn>(fn));
  }

private:
  template <typename Fn>
  void potential_neighbours(index_vector_type query_key, Fn &&fn) const {
    integral_grid<N> gen;
    gen.extents.fill(3);

    // iterate over all neighbouring grid cells
    for (auto offset : gen) {
      index_vector_type key = query_key;
      for (size_t n = 0; n < N; ++n)
        key[n] += offset[n] - 1;

      // find the cell this grid index is associated with
      auto it = _grid_to_cell.find(key);

      // nothing to do for keys with no cell
      if (it == _grid_to_cell.end())
        continue;

      // call fn for each potential neighbour
      for (size_t gi = 0; gi < _per_group.size(); ++gi) {
        for (auto ri : _per_group[gi]
                           .cell_to_entry[static_cast<size_t>(it->second)]
                           .indices) {
          std::invoke(std::forward<Fn>(fn), gi, ri);
        }
      }
    }
  }

private:
  template <typename GroupedVectorData>
  index_vector_type compute_grid_index(GroupedVectorData const &x, size_t gi,
                                       size_t ri) {
    index_vector_type result;

    for (size_t axis = 0; axis < result.size(); ++axis) {
      result[axis] = static_cast<index_type>(
          std::floor(get_element_ref(get_group_ref(x, gi),
                                     ri)[static_cast<Eigen::Index>(axis)] /
                     _radius));
    }

    return result;
  }

private:
  scalar_type _radius;

  bool _initialized = false;
  size_t _cell_count = 0;
  std::unordered_map<index_vector_type, index_type,
                     boost::hash<index_vector_type>>
      _grid_to_cell;
  std::vector<per_group> _per_group;
};

} // namespace prtcl
