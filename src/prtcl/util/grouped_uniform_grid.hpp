#pragma once

#define PRTCL_ASSERT(...)
#define PRTCL_INLINE

#include "../math.hpp"
#include "constpow.hpp"
#include "morton_order.hpp"

#include <array>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

#include <cstddef>
#include <cstdint>

#include <iostream>

#include <boost/sort/sort.hpp>

namespace prtcl {

// implementation detail {{{
namespace detail {

template <size_t N>
using grid_index_t = std::array<int32_t, N>;

template <size_t N>
struct sparse_adjacent_cell_offsets;

template <>
struct sparse_adjacent_cell_offsets<1> {
  constexpr static std::array<grid_index_t<1>, 1> value = {{
      {1},
  }};
};

template <>
struct sparse_adjacent_cell_offsets<2> {
  constexpr static std::array<grid_index_t<2>, 4> value = {{
      {1, 0},
      {0, 1},
      {1, 1},
      {1, -1},
  }};
};

template <>
struct sparse_adjacent_cell_offsets<3> {
  constexpr static std::array<grid_index_t<3>, 13> value = {{
      {1, 0, 0},
      {0, 1, 0},
      {0, 0, 1},
      {1, 1, 0},
      {1, 0, 1},
      {0, 1, 1},
      {1, 1, 1},
      {0, -1, 1},
      {1, 0, -1},
      {1, 1, -1},
      {1, -1, -1},
      {1, -1, 0},
      {1, -1, 1},
  }};
};

template <size_t N>
constexpr auto sparse_adjacent_cell_offsets_v =
    sparse_adjacent_cell_offsets<N>::value;

} // namespace detail
// }}}

template <
    size_t N,
    size_t GroupBits_ = 4, //< The number of bits of group indices.
    size_t IndexBits_ = 28 //< The number of bits of particle indices.
    >
class grouped_uniform_grid {
private:
  static_assert(1 <= N && N <= 3);

private:
  // implementation member types {{{

  enum class sorted_index : int32_t { invalid = -1 };
  enum class cell_index : int32_t { invalid = -1 };
  using grid_index = detail::grid_index_t<N>;

  // }}}

public:
  // member types {{{

  enum class raw_group : size_t {};
  static constexpr size_t max_raw_group = constpow(2, GroupBits_);

  enum class raw_index : size_t {};
  static constexpr size_t max_raw_index = constpow(2, IndexBits_);

  struct raw_grouped_index {
    size_t get_group() const { return static_cast<size_t>(group); }
    size_t get_index() const { return static_cast<size_t>(index); }

    raw_group group : GroupBits_;
    raw_index index : IndexBits_;
  };

  // }}}

public:
  grouped_uniform_grid(double radius = 1) : radius_{radius} {}

public:
  double get_radius() const { return radius_; }

  void set_radius(double radius) { radius_ = radius; }

public:
  // update(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update(GroupedVectorData const &x) {
    PRTCL_ASSERT(N == get_vector_extent(x));

    update_sorted_to_raw(x);
    update_sorted_to_cell(x);
    update_cell_to_grid_and_sorted_range(x);
    update_cell_to_adjacent_cells();
  }

private:
  // update_sorted_to_raw(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update_sorted_to_raw(GroupedVectorData const &x) {
    PRTCL_ASSERT(get_group_count(x) < max_raw_group);

    bool reset_sorted_to_raw_flag = false;

    // resize according to the current number of groups
    raw_to_sorted_.resize(get_group_count(x));

    // new number of sorted indices
    size_t new_sorted_index_count = 0;
    for (size_t group = 0; group < get_group_count(x); ++group) {
      auto &group_ref = get_group_ref(x, group);

      // old number of raw indices in group
      size_t const old_raw_index_count = raw_to_sorted_[group].size();

      // elements from groups that cannot be neighbors are not included in the
      // '...sorted...' maps
      if (not can_be_neighbor(group_ref)) {
        if (old_raw_index_count > 0)
          reset_sorted_to_raw_flag = true;

        raw_to_sorted_[group].clear();
        continue;
      }

      // new number of raw indices in group
      size_t const new_raw_index_count = get_element_count(group_ref);
      PRTCL_ASSERT(new_raw_index_count < max_raw_index);

      // resize according to the current number of raw indices in group
      raw_to_sorted_[group].resize(new_raw_index_count);

      // if the number of raw indices in _any_ group changed, flag for reset
      if (old_raw_index_count != new_raw_index_count)
        reset_sorted_to_raw_flag = true;

      // accumulate the overall number of raw indices
      new_sorted_index_count += new_raw_index_count;
    }

    if (reset_sorted_to_raw_flag) {
      // resize according to the new number of sorted indices
      sorted_to_raw_.resize(new_sorted_index_count);

      // reset sorted_to_raw_ to a valid, but unsorted permutation
      size_t n_s = 0;
      for (size_t n_g = 0; n_g < raw_to_sorted_.size(); ++n_g) {
        // the size of the raw-indexed array
        size_t const n_r_size = raw_to_sorted_[n_g].size();
#pragma omp parallel for
        for (size_t n_r = 0; n_r < n_r_size; ++n_r) {
          sorted_to_raw_[n_s + n_r] = raw_grouped_index{
              static_cast<raw_group>(n_g), static_cast<raw_index>(n_r)};
        }
        // update the base n_s index for the next iteration
        n_s += n_r_size;
      }
    }

    // sort sorted_to_raw_ using the Morton order
    boost::sort::block_indirect_sort(
        sorted_to_raw_.begin(), sorted_to_raw_.end(),
        [this, &x](auto i_gr, auto j_gr) {
          return this->compare_raw_indices(x, i_gr, j_gr);
        });

    // update raw_to_sorted_ with the sorted indices
#pragma omp parallel for
    for (size_t n_s = 0; n_s < sorted_to_raw_.size(); ++n_s) {
      auto const &i_gr = sorted_to_raw_[n_s];
      raw_to_sorted_[i_gr.get_group()][i_gr.get_index()] =
          static_cast<sorted_index>(n_s);
    }
  }

  // }}}

  // update_sorted_to_cell(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update_sorted_to_cell(GroupedVectorData const &x) {
    // resize to match the current number of sorted indices
    sorted_to_cell_.resize(sorted_to_raw_.size());

    if (0 == sorted_to_cell_.size())
      return;

    // count the number of used grid cells
    size_t used_cell_count = 1;

    // the first cell index is always 0
    sorted_to_cell_[0] = static_cast<cell_index>(0);

#pragma omp parallel for reduction(+ : used_cell_count)
    for (size_t cur_n_s = 1; cur_n_s < sorted_to_cell_.size(); ++cur_n_s) {
      auto const &cur_i_gr = sorted_to_raw_[cur_n_s];
      auto const cur_gi = compute_grid_index(x, cur_i_gr);

      size_t prv_n_s = cur_n_s - 1;
      auto const &prv_i_gr = sorted_to_raw_[prv_n_s];
      auto const prv_gi = compute_grid_index(x, prv_i_gr);

      if (cur_gi == prv_gi) {
        // 0 if the previous and the current sorted index refer to the same cell
        sorted_to_cell_[cur_n_s] = static_cast<cell_index>(0);
      } else {
        // 1 if the previous and the current sorted index refer to different
        // cells
        sorted_to_cell_[cur_n_s] = static_cast<cell_index>(1);
        used_cell_count += 1;
      }
    }

    // resize the cell_to_... arrays
    cell_to_sorted_range_.resize(used_cell_count);
    cell_to_grid_.resize(used_cell_count);
    cell_to_adjacent_cells_.resize(used_cell_count);
  }

  // }}}

  // update_cell_to_grid_and_sorted_range(GroupedVectorData) {{{

  template <typename GroupedVectorData>
  void update_cell_to_grid_and_sorted_range(GroupedVectorData const &x) {
    if (0 == cell_to_sorted_range_.size())
      return;

      // invalidate all cell_to_... arrays
#pragma omp parallel for
    for (size_t n_c = 0; n_c < cell_to_grid_.size(); ++n_c) {
#if !defined(NDEBUG)
      cell_to_sorted_range_[n_c] =
          std::make_pair(sorted_index::invalid, sorted_index::invalid);
      cell_to_grid_[n_c].fill(std::numeric_limits<int32_t>::min());
#endif // !defined(NDEBUG)
      cell_to_adjacent_cells_[n_c].fill(cell_index::invalid);
    }

    // first cell
    cell_to_sorted_range_[0].first = static_cast<sorted_index>(0);
    cell_to_grid_[0] = compute_grid_index(x, sorted_to_raw_[0]);

    for (size_t cur_n_s = 1; cur_n_s < sorted_to_raw_.size(); ++cur_n_s) {
      auto const value = static_cast<size_t>(sorted_to_cell_[cur_n_s]);

      // cumulative sum to compute the actual cell index
      size_t n_c = static_cast<size_t>(sorted_to_cell_[cur_n_s - 1]) + value;
      sorted_to_cell_[cur_n_s] = static_cast<cell_index>(n_c);

      // update the cell_to_... for the first sorted index in the cell
      if (value == 1) {
        auto const cur_i_s = static_cast<sorted_index>(cur_n_s);

        // update the cell_to_sorted_range for the previous and current cell
        cell_to_sorted_range_[n_c - 1].second = cur_i_s;
        cell_to_sorted_range_[n_c].first = cur_i_s;

        // update the grid index of the cell
        cell_to_grid_[n_c] = compute_grid_index(x, sorted_to_raw_[cur_n_s]);
      }
    }

    // last cell
    cell_to_sorted_range_[cell_to_sorted_range_.size() - 1].second =
        static_cast<sorted_index>(sorted_to_raw_.size());
  }

  // }}}

  // update_cell_to_adjacent_cells(GroupedVectorData) {{{

  void update_cell_to_adjacent_cells() {
#pragma omp parallel for
    for (size_t n_c = 0; n_c < cell_to_grid_.size(); ++n_c) {
      // enumerate all sparse adjacent cell offsets
      constexpr auto sparse_offsets = detail::sparse_adjacent_cell_offsets_v<N>;
      for (size_t sparse_offset_index = 0;
           sparse_offset_index < sparse_offsets.size(); ++sparse_offset_index) {
        grid_index adjacent_grid_index = sparse_offsets[sparse_offset_index];

        // compute the grid index of the adjacent cell
        for (size_t i = 0; i < N; ++i)
          adjacent_grid_index[i] += cell_to_grid_[n_c][i];

        if (cell_index j_c = find_cell(adjacent_grid_index);
            j_c != cell_index::invalid) {
          // update the adjacent cells of n_c
          cell_to_adjacent_cells_[n_c][sparse_offset_index] = j_c;

          // update the adjacent cells of j_c
          cell_to_adjacent_cells_[static_cast<size_t>(j_c)]
                                 [constpow(3, N) - 2 - sparse_offset_index] =
                                     static_cast<cell_index>(n_c);
        }
      }
    }
  }

  // }}}

  // find_cell(grid_index) -> cell_index {{{

  PRTCL_INLINE cell_index find_cell(grid_index const &gi) {
    return std::as_const(*this).find_cell(gi);
  }

  PRTCL_INLINE cell_index find_cell(grid_index const &gi) const {
    auto const first = cell_to_grid_.begin();
    auto const last = cell_to_grid_.end();
    auto const it =
        std::lower_bound(first, last, gi, [](auto const &lhs, auto const &rhs) {
          return morton_order(lhs, rhs);
        });

    if (it == last || *it != gi)
      return cell_index::invalid;
    else
      return static_cast<cell_index>(std::distance(first, it));
  }

  // }}}

  template <typename GroupedVectorData>
  PRTCL_INLINE bool compare_raw_indices(
      GroupedVectorData const &x, raw_grouped_index const &lhs,
      raw_grouped_index const &rhs) {
    auto lhs_gi = this->compute_grid_index(x, lhs);
    auto rhs_gi = this->compute_grid_index(x, rhs);
    return morton_order(lhs_gi, rhs_gi);
  }

  // }}}

public:
  // compute_group_permutation(group, perm) {{{

  template <typename OutputIt>
  void compute_group_permutation(size_t group, OutputIt it) const {
    for (size_t n_s = 0; n_s < sorted_to_raw_.size(); ++n_s) {
      auto i_gr = sorted_to_raw_[n_s];
      if (i_gr.get_group() == group) {
        *(it++) =
            static_cast<typename std::iterator_traits<OutputIt>::value_type>(
                i_gr.get_index());
      }
    }
  }

  // }}}

  // compute_group_permutations(range_) {{{

  template <typename Range_>
  void compute_group_permutations(Range_ &range_) const {
    for (size_t n_s = 0; n_s < sorted_to_raw_.size(); ++n_s) {
      auto i_gr = sorted_to_raw_[n_s];
      *(range_[i_gr.get_group()]++) = i_gr.get_index();
    }
  }

  // }}}

public:
  // neighbors(group, index, data, callback) {{{

  template <typename GroupedVectorData, typename Fn>
  void neighbors(
      size_t group, size_t index, GroupedVectorData const &data, Fn fn) const {
    PRTCL_ASSERT(N == get_vector_extent(data));

    double radius_squared = constpow(radius_, 2);
    potential_neighbors(
        group, index, data,
        [group, index, radius_squared, &fn, &data](size_t i_g, size_t i_r) {
          auto const distance = math::norm_squared(
              get_element_ref(get_group_ref(data, group), index) -
              get_element_ref(get_group_ref(data, i_g), i_r));
          if (distance < radius_squared) {
            std::invoke(fn, i_g, i_r);
          }
        });
  }

  // }}}

  // neighbors(position, data, callback) {{{

  template <typename X_, typename GroupedVectorData_, typename Fn_>
  void neighbors(X_ const &x_, GroupedVectorData_ const &d_, Fn_ fn_) const {
    double radius_squared = constpow(radius_, 2);
    potential_neighbors(
        x_, [radius_squared, &fn_, &x_, &d_](size_t i_g, size_t i_r) {
          auto const distance = math::norm_squared(
              x_ - get_element_ref(get_group_ref(d_, i_g), i_r));
          if (distance < radius_squared) {
            std::invoke(fn_, i_g, i_r);
          }
        });
  }

  // }}}

private:
  // potential_neighbors(cell, callback) {{{

  template <typename Fn>
  void potential_neighbors(cell_index i_c, Fn fn) const {
    if (i_c == cell_index::invalid)
      return;

    auto const &range_s = cell_to_sorted_range(i_c);

    PRTCL_ASSERT(range_s.first != sorted_index::invalid);
    PRTCL_ASSERT(range_s.second != sorted_index::invalid);

    size_t first = static_cast<size_t>(range_s.first),
           last = static_cast<size_t>(range_s.second);

    for (size_t i = first; i < last; ++i) {
      auto i_gr = sorted_to_raw(static_cast<sorted_index>(i));
      fn(i_gr.get_group(), i_gr.get_index());
    }
  }

  // }}}

private:
  // potential_neighbors(group, index, callback) {{{

  template <typename GroupedVectorData, typename Fn>
  void potential_neighbors(
      size_t group, size_t index, GroupedVectorData const &data, Fn fn) const {
    PRTCL_ASSERT(group < max_raw_group);
    PRTCL_ASSERT(index < max_raw_index);

    raw_grouped_index i_gr{
        static_cast<raw_group>(group), static_cast<raw_index>(index)};

    auto &group_ref = get_group_ref(data, i_gr.get_group());

    if (can_be_neighbor(group_ref)) {
      sorted_index i_s = raw_to_sorted(i_gr);
      cell_index i_c = sorted_to_cell(i_s);

      potential_neighbors(i_c, fn);

      for (cell_index j_c : cell_to_adjacent_cells(i_c))
        potential_neighbors(j_c, fn);
    } else {
      potential_neighbors(get_element_ref(group_ref, i_gr.get_index()), fn);
    }
  }

  // }}}

  // {{{ potential_neighbors(position, callback)

  //! Invokes the callback for each neighbor of the position.
  template <typename X_, typename Fn_>
  void potential_neighbors(X_ const &x_, Fn_ fn_) const {
    auto x_gi = x_to_gi(x_);

    if (auto x_ci = find_cell(x_gi); x_ci != cell_index::invalid) {
      // if the position is in a known cell, use the adjacent cells

      potential_neighbors(x_ci, fn_);
      for (cell_index y_ci : cell_to_adjacent_cells_[static_cast<size_t>(x_ci)])
        potential_neighbors(y_ci, fn_);
    } else {
      // if the position is NOT in a known cell, iterate over all adjacent cells
      // by their respective grid index

      // enumerate all sparse adjacent cell offsets
      for (auto const &offset : detail::sparse_adjacent_cell_offsets_v<N>) {
        // grid index of an adjacent cell
        grid_index y_gi = x_gi;

        // offset applied by addition
        for (size_t i = 0; i < N; ++i)
          y_gi[i] += offset[i];

        potential_neighbors(find_cell(y_gi), fn_);

        // offset applied by subtraction
        for (size_t i = 0; i < N; ++i)
          y_gi[i] -= 2 * offset[i];

        potential_neighbors(find_cell(y_gi), fn_);
      }
    }
  }

  // }}}

private:
  // raw_to_sorted : G × R -> S {{{

  auto const &raw_to_sorted(raw_grouped_index const &i_gr) const {
    PRTCL_ASSERT(i_gr.get_group() < max_raw_group);
    PRTCL_ASSERT(i_gr.get_index() < max_raw_index);
    return raw_to_sorted_[static_cast<size_t>(i_gr.group)]
                         [static_cast<size_t>(i_gr.index)];
  }

  std::vector<std::vector<sorted_index>> raw_to_sorted_;

  // }}}

  // sorted_to_raw : S -> G × R {{{

  auto const &sorted_to_raw(sorted_index const &i_s) const {
    PRTCL_ASSERT(i_s != sorted_index::invalid);
    return sorted_to_raw_[static_cast<size_t>(i_s)];
  }

  std::vector<raw_grouped_index> sorted_to_raw_;

  // }}}

  // sorted_to_cell : S -> C {{{

  auto const &sorted_to_cell(sorted_index const &i_s) const {
    PRTCL_ASSERT(i_s != sorted_index::invalid);
    return sorted_to_cell_[static_cast<size_t>(i_s)];
  }

  std::vector<cell_index> sorted_to_cell_;

  // }}}

  // cell_to_sorted_range : C -> S^2 {{{

  auto const &cell_to_sorted_range(cell_index const &i_c) const {
    PRTCL_ASSERT(i_c != cell_index::invalid);
    return cell_to_sorted_range_[static_cast<size_t>(i_c)];
  }

  std::vector<std::pair<sorted_index, sorted_index>> cell_to_sorted_range_;

  // }}}

  // cell_to_adjacent_cells : C -> C^(3^d - 1) {{{

  auto const &cell_to_adjacent_cells(cell_index const &i_c) const {
    PRTCL_ASSERT(i_c != cell_index::invalid);
    return cell_to_adjacent_cells_[static_cast<size_t>(i_c)];
  }

  std::vector<std::array<cell_index, static_cast<size_t>(constpow(3, N) - 1)>>
      cell_to_adjacent_cells_;

  // }}}

  // cell_to_grid_index : C -> Z^d {{{

  auto const &cell_to_grid_index(cell_index const &i_c) const {
    PRTCL_ASSERT(i_c != cell_index::invalid);
    return cell_to_grid_[static_cast<size_t>(i_c)];
  }

  std::vector<grid_index> cell_to_grid_;

  // }}}

private:
  // compute_grid_index(...) -> ... {{{

  template <typename GroupedVectorData>
  grid_index
  compute_grid_index(GroupedVectorData const &x, raw_grouped_index i_gr) {
    return x_to_gi(
        get_element_ref(get_group_ref(x, i_gr.get_group()), i_gr.get_index()));
  }

  // }}}

  // {{{ x_to_gi : O -> Z^d

  //! Maps a position to it's corresponding cell index.
  template <typename X_>
  grid_index x_to_gi(X_ const &x) const {
    grid_index result;
    using single_grid_index = typename grid_index::value_type;

    for (size_t axis = 0; axis < result.size(); ++axis) {
      auto const component = static_cast<double>(x[static_cast<int>(axis)]);
      result[axis] =
          static_cast<single_grid_index>(std::floor(component / radius_));
    }

    return result;
  }

  // }}}

private:
  double radius_;
};

} // namespace prtcl

// NOTE:
//    The public interface of grouped_uniform_grid is inspired by
//    https://github.com/InteractiveComputerGraphics/CompactNSearch
//    Specifically the type CompactNSearch::NeighbourhoodSearch.
//    The algorithms used internally are different though.
