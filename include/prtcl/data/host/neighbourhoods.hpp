#pragma once

#include "../../math/traits/host_math_traits.hpp"

#include <esph/compatibility/span.hpp>
#include <esph/constpow.hpp>
#include <esph/morton_order.hpp>
#include <esph/types.hpp>
#include <esph/utility/index_vector_grid.hpp>

#include <algorithm>
#include <array>
#include <numeric>
#include <unordered_map>
#include <vector>

#include <cstdint>

#include <boost/sort/block_indirect_sort/block_indirect_sort.hpp>

namespace prtcl {

template <typename T, size_t N, typename I = std::int32_t>
class neighbourhoods {
  using math_traits = host_math_traits<T, N>;
  using scalar_type = typename math_traits::scalar_type;

  // using vector_type = vector_t<T, N>;
  // using index_vector_type = vector_t<index_t, N>;

  using index_type = I;
  using index_vector_type = std::array<index_type, N>;

  struct list_entry {
    std::array<index_type, 7> indices;
    index_type next_entry;
  };

public:
  neighbourhoods() = default;

public:
  scalar_type get_radius() const { return _radius; }

  void set_radius(scalar_type value) { _radius = value; }

public:
  /// Clears the neighbourhood information. This is useful if the neighbourhood
  /// information should be fully rebuilt.
  void clear() {
    sorted_.clear();
    cells_.clear();
  }

  template <typename PositionF, typename = std::enable_if_t<std::is_invocable_r<
                                    vector_type, PositionF, index_t>::value>>
  void update(index_t count, PositionF position) {
    // in case the number of positions has changed, resize and re-initialize the
    // indices
    if (sorted_.size() != static_cast<std::size_t>(count)) {
      sorted_.resize(static_cast<std::size_t>(count));
      std::iota(sorted_.begin(), sorted_.end(), 0);
    }

    // if the diameter of the grid is zero, clear the cell map and bail because
    // without the diameter the grid cell of a position cannot be computed
    // TODO: add documentation that no neighbours will be found if the diameter
    //       isn't set
    if (neighbourhood_radius_ == scalar_type{}) {
      cells_.clear();
      return;
    }

    // re-sort the indices according to the grid-cell order
    boost::sort::block_indirect_sort(
        sorted_.begin(), sorted_.end(), [&position, this](auto lhs, auto rhs) {
          return morton_order{}(compute_cell(position(lhs)),
                                compute_cell(position(rhs)));
        });

    { // traverse the positions once and record the smallest index for each cell
      // TODO: can this be parallelized? leave a note here for future reference
      auto first = sorted_.begin(), last = sorted_.end();
      auto it = first;
      while (it != last) {
        auto it_cell = compute_cell(position(*it));
        // save the index of the first index in each cell
        cells_[it_cell] = static_cast<index_t>(std::distance(first, it));
        // advance to the next cell
        it =
            std::find_if_not(it, last, [&it_cell, &position, this](auto index) {
              return it_cell == compute_cell(position(index));
            });
      }
    }
  }

  template <typename PositionF, typename OutputIt,
            typename = std::enable_if_t<
                std::is_invocable_r<vector_type, PositionF, index_t>::value>>
  void neighbours(vector_type where, PositionF position, OutputIt out) {
    index_vector_type base = compute_cell(where) - make_index_vector<N>(1);
    for (auto cell : make_index_vector_grid(make_index_vector<N>(3))) {
      cell = base + cell;
      // find the (possibly) first index into the cell
      auto it = cells_.find(cell);
      // check for cells that aren't indexed
      if (it == cells_.end())
        continue;
      index_t first = it->second;
      // check that the first index is valid
      if (first >= static_cast<index_t>(sorted_.size()))
        continue;
      // iterate over all indices until one is not in the current cell
      for (index_t index = first; index < static_cast<index_t>(sorted_.size());
           ++index) {
        auto index_position =
            position(sorted_[static_cast<std::size_t>(index)]);
        auto index_cell = compute_cell(index_position);
        // check if the position is in another cell
        if (index_cell != cell)
          break;
        // check if the position is in range
        if (norm(index_position - where) <=
            neighbourhood_radius_ + epsilon<scalar_type>())
          *out = sorted_[static_cast<std::size_t>(index)];
      }
    }
  }

  /// Returns an array reference to the permutation of indices this
  /// neighbourhood search uses internally. This can be used to sort data for
  /// better locality of reference in the neighbourhood queries.
  span<const index_t> get_index_permutation() const {
    return {sorted_.data(), sorted_.size()};
  }

  /// Calling this function resets the index permutation to the identity
  /// mapping. This **must** be done after permuting the data according to the
  /// permutation given by get_index_permutation().
  void index_permutation_applied() {
    std::iota(sorted_.begin(), sorted_.end(), 0);
  }

private:
  template <typename GroupedVectorData>
  index_vector_type compute_grid_index(GroupedVectorData const &x, size_t g,
                                       size_t i) {
    index_vector_type result;

    for (size_t axis = 0; axis < result.size(); ++axis) {
      result[axis] = static_cast<index_type>(
          std::floor(get_element_ref(get_group_ref(x, g),
                                     i)[static_cast<Eigen::Index>(axis)] /
                     radius_));
    }

    return result;
  }

private:
  scalar_type _radius;

  std::vector<std::vector<index_type>> group_to_cell_to_list_;

  std::vector<list_entry> list_;
  std::vector<index_type> free_list_;

  std::unordered_map<index_vector_type, index_type> cells_;
};

} // namespace prtcl
